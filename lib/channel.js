var _ = require("underscore");
var util = require("util");

util.inherits(Channel, require("events").EventEmitter);
function Channel(telopt, telnet) {
  telnet.bind(telopt, _.bind(this._onEvent, this));
  
  this.telopt = telopt;
  this.telnet = telnet;
}

_.extend(Channel.prototype, {
  _onEvent: function(ev) {
    switch (ev.type) {
      case "toggle":
        this.emit("toggle", ev.where, ev.status != 0);
        break;
      case "focus":
        this.emit("focus", ev.focus != 0);
        break;
      case "text":
        this.emit("text", ev.text);
        break;
    }
  }
});

module.exports = Channel;
