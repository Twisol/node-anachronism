var Native = require("../build/default/anachronism").Telnet;
var EventEmitter = require("events").EventEmitter;
var _ = require("underscore");
var util = require("util");

util.inherits(AllocError, Error);
function AllocError(message) {
  this.name = "AllocError";
  this.message = message || "Allocation failed";
}

util.inherits(Telnet, EventEmitter);
function Telnet() {
  this.callbacks = [];
  this.native = new Native(
    _.bind(this._onEvent, this),
    _.bind(this._onTeloptEvent, this));
}

_.extend(Telnet.prototype, {
  receive: function(data) {
    var bytes_used = this.native.receive(data);
    if (bytes_used == null)
      throw new AllocError();
    
    var rest = new Buffer(data.length - bytes_used);
    data.copy(rest, 0, bytes_used);
    return rest;
  },
  
  interruptParser: function() {
    this.native.interruptParser();
  },
  
  
  sendText: function(text) {
    this.native.sendText(text);
  },
  
  sendCommand: function(command) {
    this.native.sendCommand(command);
  },
  
  sendSubnegotiation: function(telopt, text) {
    this.native.sendSubnegotiation(telopt, text);
  },
  
  
  bind: function(telopt, callback) {
    var callbacks = this.callbacks;
    if (!callbacks[telopt]) {
      callbacks[telopt] = callback;
      return true;
    } else {
      return false;
    }
  },
  
  unbind: function(telopt) {
    delete this.callbacks[telopt];
  },
  
  
  requestEnableLocal: function(telopt, opts) {
    this.native.requestEnableLocal(telopt, (opts) ? !!opts.lazy : false);
  },
  requestEnableRemote: function(telopt, opts) {
    this.native.requestEnableRemote(telopt, (opts) ? !!opts.lazy : false);
  },
  requestDisableLocal: function(telopt) {
    this.native.requestDisableLocal(telopt);
  },
  requestDisableRemote: function(telopt) {
    this.native.requestDisableRemote(telopt);
  },
  
  isLocalEnabled: function(telopt) {
    return this.native.isLocalEnabled(telopt);
  },
  isRemoteEnabled: function(telopt) {
    return this.native.isRemoteEnabled(telopt);
  },
  
  
  _onEvent: function(ev) {
    switch (ev.type) {
      case "command":
        this.emit("command", ev.command);
        break;
      case "warning":
        this.emit("warning", ev.message, ev.position);
        break;
      case "text":
        this.emit("text", ev.text);
        break;
      case "send":
        this.emit("send", ev.data);
        break;
    }
  },
  
  _onTeloptEvent: function(telopt, ev) {
    var callback = this.callbacks[telopt];
    if (callback) {
      callback(ev);
    }
  },
});

module.exports = Telnet;
