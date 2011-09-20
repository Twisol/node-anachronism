var EventEmitter = require("events").EventEmitter;
var _ = require("underscore");
var Class = require("class-42");


var Channel = Class.extend(EventEmitter, {
  initialize: function(telopt, api) {
    this.telopt = telopt;
    this.api = api;
    this.handlers = {
      local: function() { return false; },
      remote: function() { return false; }
    };
    
    _.bindAll(this, "_onEvent", "_onNegotiateEvent");
  },
  
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
  },
  
  _onNegotiateEvent: function(where) {
    return this.handlers[where]();
  },
  
  listen: function(handlers) {
    if (handlers.local !== undefined) {
      if (_.isBoolean(handlers.local))
        this.handlers.local = function() {return handlers.local};
      else
        this.handlers.local = handlers.local;
    }
    
    if (handlers.remote !== undefined) {
      if (_.isBoolean(handlers.remote))
        this.handlers.remote = function() {return handlers.remote};
      else
        this.handlers.remote = handlers.remote;
    }
    
    return this;
  },
  
  send: function(data) {
    this.api.sendSubnegotiation(this.telopt, data);
  },
  
  activate: function(where) {
    if (where === "local") {
      this.api.requestEnableLocal(this.telopt, true);
    } else if (where === "remote") {
      this.api.requestEnableRemote(this.telopt, true);
    }
  },
  
  deactivate: function(where) {
    if (where === "local") {
      this.api.requestDisableLocal(this.telopt, false);
    } else if (where === "remote") {
      this.api.requestDisableRemote(this.telopt, false);
    }
  },
  
  active: function(where) {
    if (where == "local") {
      return this.api.requestStatusLocal(this.telopt);
    } else if (where === "remote") {
      return this.api.requestStatusRemote(this.telopt);
    }
  }
});

module.exports = Channel;
