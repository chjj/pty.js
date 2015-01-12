var assert = require('assert');
var pty = require('../');
var mocha = require('mocha');

var tests = [
  {
    name: 'should be correctly setup',
    command: [ 'children/void.js' ],
    options: { cwd: __dirname },
    exitCode: 0,
    test: function () {
      assert.equal(this.file, process.execPath);
    }
  }, {
    name: 'should support stdin',
    command: [ 'children/stdin.js' ],
    options: { cwd: __dirname },
    exitCode: 0,
    test: function () {
      this.write('☃');
    }
  }, {
    name: 'should support resize',
    command: [ 'children/resize.js' ],
    options: { cwd: __dirname },
    exitCode: 0,
    test: function () {
      this.resize(100, 100);
    }
  }, {
    name: 'should change uid/gid',
    command: [ 'children/uidgid.js' ],
    options: { cwd: __dirname, uid: 777, gid: 777 },
    exitCode: 0,
    test: function () {}
  }, {
    name: 'should report exitCode',
    command: [ 'children/exitCode.js' ],
    options: { cwd: __dirname },
    test: function () {},
    exitCode: 5
  }
];

describe('Pty', function() {
  tests.forEach(function (testCase) {
    if (testCase.options.uid && testCase.options.gid && (process.platform == 'win32' || process.getgid() !== 0)) {
      // Skip tests that contains user impersonation if we are not able to do so.
      return it.skip(testCase.name);
    }
    it(testCase.name, function (done) {
      var term = pty.fork(process.execPath, testCase.command, testCase.options);
      term.pipe(process.stderr);

      term.on('close', function () {
        assert.equal(term.status, testCase.exitCode);
        done();
      });

      // Wait for pty to be ready
      setTimeout(testCase.test.bind(term), 1000);
    });
  });
});

describe('The SIGCHLD handler', function () {
  it('should not interfere with child_process', function (done) {
    this.timeout(500);
    var spawn = require('child_process').spawn;
    var proc = spawn('false')
    proc.on('close', function (code) {
      assert.equal(code, 1);
      done();
    });
  });
});
