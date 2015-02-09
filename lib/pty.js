/**
 * pty.js
 * Copyright (c) 2012, Christopher Jeffrey (MIT License)
 * Binding to the pseudo terminals.
 */

var net = require('net');
var tty = require('tty');
var extend = require('extend');
var pty = require('../build/Release/pty.node');
var stream = require('stream');
var util = require('util');

/**
 * Terminal
 */

// Example:
//  var term = new Terminal('bash', [], {
//    name: 'xterm-color',
//    cols: 80,
//    rows: 24,
//    cwd: process.env.HOME,
//    env: process.env
//  });

function Terminal(file, args, opt) {
  Terminal.super_.call(this, opt);
  if (!(this instanceof Terminal)) {
    return new Terminal(file, args, opt);
  }


  var self = this
    , env
    , cwd
    , name
    , cols
    , rows
    , term;

  // backward compatibility
  if (typeof args === 'string') {
    opt = {
      name: arguments[1],
      cols: arguments[2],
      rows: arguments[3],
      cwd: process.env.HOME
    };
    args = [];
  }

  // arguments
  args = args || [];
  file = file || 'sh';
  opt = opt || {};

  cols = opt.cols || 80;
  rows = opt.rows || 24;

  opt.env = opt.env || process.env;
  env = extend({}, opt.env);

  if (opt.env === process.env) {
    // Make sure we didn't start our
    // server from inside tmux.
    delete env.TMUX;
    delete env.TMUX_PANE;

    // Make sure we didn't start
    // our server from inside screen.
    // http://web.mit.edu/gnu/doc/html/screen_20.html
    delete env.STY;
    delete env.WINDOW;

    // Delete some variables that
    // might confuse our terminal.
    delete env.WINDOWID;
    delete env.TERMCAP;
    delete env.COLUMNS;
    delete env.LINES;
  }

  // Could set some basic env vars
  // here, if they do not exist:
  // USER, SHELL, HOME, LOGNAME, WINDOWID

  cwd = opt.cwd || process.cwd();
  name = opt.name || env.TERM || 'xterm';
  env.TERM = name;

  env = environ(env);

  // fork
  term = opt.uid && opt.gid
    ? pty.fork(file, args, env, cwd, cols, rows, opt.uid, opt.gid)
    : pty.fork(file, args, env, cwd, cols, rows);

  this.socket = new tty.ReadStream(term.fd);
  this.socket.setEncoding('utf8');
  this.socket.resume();

  // setup
  this.socket.on('error', function(err) {
    // close
    self.close();

    // EIO, happens when someone closes our child
    // process: the only process in the terminal.
    // node < 0.6.14: errno 5
    // node >= 0.6.14: read EIO
    if (err.code) {
      if (~err.code.indexOf('errno 5')
          || ~err.code.indexOf('EIO')) return;
    }

    // throw anything else
    if (self.listeners('error').length < 2) {
      throw err;
    }
  });

  this.pid = term.pid;
  this.fd = term.fd;
  this.pty = term.pty;

  this.file = file;
  this.name = name;
  this.cols = cols;
  this.rows = rows;

  this.readable = true;
  this.writable = true;

  Terminal.total++;
  this.socket.on('close', function() {
    Terminal.total--;
    self.close();
    self.emit('exit', null);
  });

  env = null;
}
util.inherits(Terminal, stream.Duplex);


Terminal.fork =
Terminal.spawn =
Terminal.createTerminal = function(file, args, opt) {
  return new Terminal(file, args, opt);
};

/**
 * openpty
 */

Terminal.open = function(opt) {
  var self = Object.create(Terminal.prototype)
    , opt = opt || {};

  if (arguments.length > 1) {
    opt = {
      cols: arguments[1],
      rows: arguments[2]
    };
  }

  var cols = opt.cols || 80
    , rows = opt.rows || 24
    , term;

  // open
  term = pty.open(cols, rows);

  self.master = new tty.ReadStream(term.master);
  self.master.setEncoding('utf8');
  self.master.resume();

  self.slave = new tty.ReadStream(term.slave);
  self.slave.setEncoding('utf8');
  self.slave.resume();

  self.socket = self.master;
  self.pid = null;
  self.fd = term.master;
  self.pty = term.pty;
  self.canPush = false;

  self.file = process.argv[0] || 'node';
  self.name = process.env.TERM || '';
  self.cols = cols;
  self.rows = rows;

  self.readable = true;
  self.writable = true;

  self.socket.on('error', function(err) {
    self._close();
    if (self.listeners('error').length < 2) {
      throw err;
    }
  });

  Terminal.total++;
  self.socket.on('close', function() {
    Terminal.total--;
    self.close();
  });

  self.socket.on('readable', function() {
    if(self._canPush)
      self._canPush = self.push(self.socket.read());
  });

  return self;
};

/**
 * Total
 */

// Keep track of the total
// number of terminals for
// the process.
Terminal.total = 0;

/**
 * Events
 */

Terminal.prototype._read = function() {
  this._canPush = this.push(this.socket.read());
}

Terminal.prototype._write = function(chunk, encoding, callback) {
  this.socket.write(chunk, encoding, callback);
}

Terminal.prototype.__defineGetter__('stdin', function() {
  return this;
});

Terminal.prototype.__defineGetter__('stdout', function() {
  return this;
});

Terminal.prototype.__defineGetter__('stderr', function() {
  throw new Error('No stderr.');
});

/**
 * TTY
 */

Terminal.prototype.resize = function(cols, rows) {
  cols = cols || 80;
  rows = rows || 24;

  this.cols = cols;
  this.rows = rows;

  pty.resize(this.fd, cols, rows);
};

Terminal.prototype.destroy = function() {
  var self = this;

  // close
  this._close();

  // Need to close the read stream so
  // node stops reading a dead file descriptor.
  // Then we can safely SIGHUP the shell.
  this.socket.once('close', function() {
    self.kill('SIGHUP');
  });

  this.socket.destroy();
};

Terminal.prototype.kill = function(sig) {
  try {
    process.kill(this.pid, sig || 'SIGHUP');
  } catch(e) {
    ;
  }
};

Terminal.prototype.redraw = function() {
  var self = this
    , cols = this.cols
    , rows = this.rows;

  // We could just send SIGWINCH, but most programs will
  // ignore it if the size hasn't actually changed.

  this.resize(cols + 1, rows + 1);

  setTimeout(function() {
    self.resize(cols, rows);
  }, 30);
};

Terminal.prototype.close = Terminal.prototype.end;

Terminal.prototype.__defineGetter__('process', function() {
  return pty.process(this.fd, this.pty) || this.file;
});

/**
 * Helpers
 */

function environ(env) {
  var keys = Object.keys(env || {})
    , l = keys.length
    , i = 0
    , pairs = [];

  for (; i < l; i++) {
    pairs.push(keys[i] + '=' + env[keys[i]]);
  }

  return pairs;
}

/**
 * Expose
 */

module.exports = exports = Terminal;
exports.Terminal = Terminal;
exports.native = pty;
