var EventEmitter = require("events").EventEmitter;
var _ = require("underscore");
var Class = require("class-42");

var NetworkBuffer = Class.extend(EventEmitter, {
  initialize: function() {
    this.buffer = [];
    this.bytes = 0;
  },
  
  write: function(data) {
    this.buffer.push(data);
    this.bytes += data.length;
  },
  
  flush: function() {
    var buffer = this.buffer;
    var data = new Buffer(this.bytes);
    
    var left = 0;
    var chunk;
    for (var i = 0, len = buffer.length; i < len; ++i) {
      chunk = buffer[i];
      chunk.copy(data, left);
      left += chunk.length;
    }
    
    this.buffer = [];
    this.bytes = 0;
    
    this.emit("data", data);
  },
});

var Service = Class.extend(EventEmitter, {
  initialize: function(telnet, options) {
    var channel = telnet.getChannel(options.telopt || this.telopt);
    if (options !== undefined) channel.listen(options);
    
    if (this.onToggle) channel.on("toggle", this.onToggle.bind(this));
    
    // Buffer incoming data by default until the telopt loses focus.
    // If more immediate access to the data is needed, set
    // unbuffered = true in the subclass
    if (this.unbuffered) {
      if (this.onText) channel.on("text", this.onText.bind(this));
      if (this.onFocus) channel.on("focus", this.onFocus.bind(this));
    } else {
      if (this.onText) {
        var dataBuffer = new NetworkBuffer();
        dataBuffer.on("data", this.onText.bind(this));
        
        channel.on("text", dataBuffer.write.bind(dataBuffer));
        channel.on("focus", function(focused) {
          if (!focused) dataBuffer.flush();
        });
      }
    }
    
    this.channel = channel;
    this.options = options;
  },
  
  send: function(data) {
    return this.channel.send(data);
  },
  
  activate: function(where) {
    return this.channel.activate(where);
  },
  
  deactivate: function(where) {
    return this.channel.deactivate(where);
  },
  
  active: function(where) {
    return this.channel.active(where);
  },
});

module.exports = Service;
