var EventEmitter = require("events").EventEmitter;
var Stream = require("stream").Stream;
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

var Telnet = Class.extend(Stream, {
  initialize: function(source) {
    source.on("data", this.receive.bind(this));
    source.on("close", this.emit.bind(this, "close"));
    source.on("drain", this.emit.bind(this, "drain"));

    source.on("error", function(err) {this.destroy(); this.emit("error", err)});
    source.on("end", function() {this.destroy(); this.emit("end", err)});
    
    this.channels = [];
    this.api = new Native(
       this._onEvent.bind(this),
       this._onTeloptEvent.bind(this),
       this._onNegotiateEvent.bind(this));

    this.source = source;
    this.readable = true;
    this.writable = true;
    this.encoding = null;
    this._writeResult = null;
  },
  
  receive: function(data) {
    var bytes_used = this.api.receive(data);
    if (!bytes_used)
      throw new AllocError();
    
    var rest = new Buffer(data.length - bytes_used);
    data.copy(rest, 0, bytes_used);
    this.emit("partial", rest);
  },
  
  interrupt: function() {
    this.api.interruptParser();
  },
  
  write: function(text, encoding) {
    if (encoding !== undefined) {
      text = new Buffer(text, encoding);
    }
    
    this.api.sendText(text);
    return this._writeResult;
  },
  
  command: function(command) {
    this.api.sendCommand(command);
    if (callback) callback();
  },
  
  getChannel: function(telopt) {
    var channel = this.channels[telopt];
    if (!channel) {
      channel = new Channel(telopt, this.api);
      this.channels[telopt] = channel;
    }
    return channel;
  },

  setEncoding: function(encoding) {
    this.encoding = encoding;
  },

  destroy: function() {
    this.readable = this.writable = false;
    this.source.destroy();
  },

  destroySoon: function() {
    this.readable = this.writable = false;
    this.source.destroySoon();
  },

  end: function(text, encoding) {
    if (text !== undefined) {
      this.write(text, encoding);
    }
    this.source.end();
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
        this.emit("data", ev.text);
        break;
      case "send":
        var data = ev.data;
        if (this.encoding !== null) {
          data = data.toString(this.encoding);
        }
        this._writeResult = this.source.write(data);
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
