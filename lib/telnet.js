var EventEmitter = require("events").EventEmitter;
var _ = require("underscore");
var Class = require("class-42");

var Native = require("../build/default/anachronism").Telnet;
var Channel = require("./channel.js");

var AllocError = Class.extend(Error, {
  name: "AllocError",
  message: "Allocation failed",
  
  initialize: function(message) {
    if (message !== undefined)
      this.message = message;
  }
});

var Telnet = Class.extend(EventEmitter, {
  initialize: function() {
    this.channels = [];
    this.api = new Native(
       this._onEvent.bind(this),
       this._onTeloptEvent.bind(this),
       this._onNegotiateEvent.bind(this));
  },
  
  receive: function(data) {
    var bytes_used = this.api.receive(data);
    if (!bytes_used)
      throw new AllocError();
    
    var rest = new Buffer(data.length - bytes_used);
    data.copy(rest, 0, bytes_used);
    return rest;
  },
  
  interrupt: function() {
    this.api.interruptParser();
  },
  
  send: function(text) {
    this.api.sendText(text);
  },
  
  command: function(command) {
    this.api.sendCommand(command);
  },
  
  getChannel: function(telopt) {
    var channel = this.channels[telopt];
    if (!channel) {
      channel = new Channel(telopt, this.api);
      this.channels[telopt] = channel;
    }
    return channel;
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
    this.getChannel(telopt)._onEvent(ev);
  },
  
  _onNegotiateEvent: function(telopt, where) {
    return this.getChannel(telopt)._onNegotiateEvent(where);
  }
});

module.exports = Telnet;
