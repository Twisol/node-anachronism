var EventEmitter = require("events").EventEmitter;
var _ = require("underscore");
var Class = require("class-42");


var Channel = Class.extend(EventEmitter, {
  initialize: function(telopt, api) {
    this._telopt = telopt;
    this._api = api;
    this._encoding = null;
    
    this.handlers = {
      local: function() { return false; },
      remote: function() { return false; }
    };

    this.writable = false;
    this.readable = false;
    
    _.bindAll(this, "_onEvent", "_onNegotiate");
  },
  
  _onEvent: function(ev) {
    switch (ev.type) {
      case "toggle":
        if (ev.status !== 0) {
          this.writable = this.readable = true;
        } else if (this.active("local") == this.active("remote")) {
          this.writable = this.readable = false;
        }
        
        this.emit("toggle", ev.where, ev.status !== 0);
        break;
      case "focus":
        this.emit("focus", ev.focus !== 0);
        break;
      case "text":
        var data = ev.text;
        if (this._encoding !== null) {
          data = data.toString(this._encoding);
        }
        this.emit("data", data);
        break;
    }
  },
  
  _onNegotiate: function(where) {
    return this.handlers[where]();
  },

  setEncoding: function(encoding) {
    this._encoding = encoding;
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
  
  write: function(data, encoding) {
    if (!Buffer.isBuffer(data)) {
      data = new Buffer(data, encoding);
    }
    this._api.sendSubnegotiation(this._telopt, data);
  },
  
  activate: function(where) {
    if (where === "local") {
      this._api.requestEnableLocal(this._telopt, true);
    } else if (where === "remote") {
      this._api.requestEnableRemote(this._telopt, true);
    }
  },
  
  deactivate: function(where) {
    if (where === "local") {
      this._api.requestDisableLocal(this._telopt, false);
    } else if (where === "remote") {
      this._api.requestDisableRemote(this._telopt, false);
    }
  },
  
  active: function(where) {
    if (where === "local") {
      return this._api.requestStatusLocal(this._telopt);
    } else if (where === "remote") {
      return this._api.requestStatusRemote(this._telopt);
    }
  }
});

module.exports = Channel;
