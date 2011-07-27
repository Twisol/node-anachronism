#include <cassert>
#include <node_buffer.h>

#include "channel.hpp"
#include "nvt.hpp"

using namespace v8;
using namespace node;
using anachronism::Channel;


Persistent<FunctionTemplate> Channel::constructor_template;

// Check if something is a Channel
bool Channel::HasInstance(Handle<Value> value) {
  return constructor_template->HasInstance(value);
}

// Initialize the Channel type
void Channel::Init(v8::Handle<v8::Object> target) {
  // Create a function template for this class
  Local<FunctionTemplate> ctor(FunctionTemplate::New(&Channel::New));
  
  ctor->Inherit(EventEmitter::constructor_template);
  
  // Make space to store a reference to a telnet_channel instance.
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(String::NewSymbol("Channel"));
  
  // Attach methods to the prototype.
  NODE_SET_PROTOTYPE_METHOD(ctor, "attach", &Channel::Attach);
  
  // Keep the function template from getting GC'd
  Channel::constructor_template = Persistent<FunctionTemplate>::New(ctor);
  
  // Attach the constructor function to the namespace provided.
  target->Set(String::NewSymbol("Channel"), ctor->GetFunction());
}


Channel::Channel() {
  this->channel_ = telnet_channel_new(&Channel::OnToggle,
                                      &Channel::OnData,
                                      (void*)this);
}
Channel::~Channel() {
  telnet_channel_free(this->channel_);
}

v8::Handle<v8::Value> Channel::New(const v8::Arguments& args) {
  HandleScope scope;
  
  Channel* channel = new Channel();
  channel->Wrap(args.This());
  return args.This();
}


// Listen to a "port" on an NVT
telnet_error Channel::Attach(NVT* nvt, short id) {
  return telnet_channel_register(this->channel_,
                                 nvt->nvt_,
                                 id,
                                 TELNET_CHANNEL_OFF,
                                 TELNET_CHANNEL_OFF);
}

v8::Handle<v8::Value> Channel::Attach(const v8::Arguments& args) {
  HandleScope scope;
  
  if (!NVT::HasInstance(args[0])) {
    return ThrowException(Exception::Error(String::New("Argument #1 must be an NVT.")));
  } else if (!args[1]->IsNumber()) {
    return ThrowException(Exception::Error(String::New("Argument #2 must be a number.")));
  }
  
  Channel* channel = ObjectWrap::Unwrap<Channel>(args.This());
  NVT* nvt = ObjectWrap::Unwrap<NVT>(args[0]->ToObject());
  short id = (short)args[1]->Int32Value();
  
  channel->Attach(nvt, id);
  
  return args.This();
}


// Open or close the channel in either direction
telnet_error Channel::ToggleLocal(telnet_channel_mode mode) {
  return telnet_channel_toggle(this->channel_, TELNET_CHANNEL_LOCAL, mode);
}
telnet_error Channel::ToggleRemote(telnet_channel_mode mode) {
  return telnet_channel_toggle(this->channel_, TELNET_CHANNEL_REMOTE, mode);
}

v8::Handle<v8::Value> Channel::Toggle(const v8::Arguments& args) {
  HandleScope scope;
  if (!args[0]->IsObject()) {
    return ThrowException(Exception::Error(String::New("Argument #1 must be an object.")));
  }
  
  Channel* channel = ObjectWrap::Unwrap<Channel>(args.This());
  
  Handle<Object> options = args[0]->ToObject();
  Handle<Value> local_value = options->Get(String::NewSymbol("local"));
  Handle<Value> remote_value = options->Get(String::NewSymbol("remote"));
  
  // Toggle the local mode if we need to
  if (!local_value->IsUndefined()) {
    telnet_channel_mode mode;
    if (local_value->IsString() && local_value->ToString()->StrictEquals(String::NewSymbol("lazy"))) {
      mode = TELNET_CHANNEL_LAZY;
    } else {
      mode = (local_value->BooleanValue()) ? TELNET_CHANNEL_ON : TELNET_CHANNEL_OFF;
    }
    channel->ToggleLocal(mode);
  }
  
  // Toggle the remote mode if we need to
  if (!remote_value->IsUndefined()) {
    telnet_channel_mode mode;
    if (remote_value->IsString() && remote_value->ToString()->StrictEquals(String::NewSymbol("lazy"))) {
      mode = TELNET_CHANNEL_LAZY;
    } else {
      mode = (remote_value->BooleanValue()) ? TELNET_CHANNEL_ON : TELNET_CHANNEL_OFF;
    }
    channel->ToggleRemote(mode);
  }
  
  return args.This();
}


void Channel::OnToggle(telnet_channel_mode on, telnet_channel_provider who) {
  HandleScope scope;
  
  Local<String> str;
  if (who == TELNET_CHANNEL_LOCAL) {
    str = String::NewSymbol("local");
  } else {
    str = String::NewSymbol("remote");
  }
  
  Handle<Value> args[1] = {str};
  if (on) {
    this->Emit(String::NewSymbol("open"), 1, args);
  } else {
    this->Emit(String::NewSymbol("close"), 1, args);
  }
}

void Channel::OnToggle(telnet_channel* channel,
                       telnet_channel_mode on,
                       telnet_channel_provider who) {
  Channel* self = NULL;
  telnet_channel_get_userdata(channel, (void**)&self);
  self->OnToggle(on, who);
}


void Channel::OnData(telnet_channel_event type,
                     const telnet_byte* data,
                     size_t length) {
  switch (type) {
    case TELNET_CHANNEL_EV_BEGIN:
      this->Emit(String::NewSymbol("focus"), 0, NULL);
      break;
    case TELNET_CHANNEL_EV_END:
      this->Emit(String::NewSymbol("blur"), 0, NULL);
      break;
    case TELNET_CHANNEL_EV_DATA: {
      Handle<Value> args[1] = {
        Local<Value>::New(Buffer::New((char*)data, length)->handle_),
      };
      this->Emit(String::NewSymbol("data"), 1, args);
      break;
    }
  }
}

void Channel::OnData(telnet_channel* channel,
                     telnet_channel_event type,
                     const telnet_byte* data,
                     size_t length) {
  Channel* self = NULL;
  telnet_channel_get_userdata(channel, (void**)&self);
  self->OnData(type, data, length);
}

/*

var anachronism = require("anachronism");
var NVT = anachronism.NVT;
var Channel = anachronism.Channel;

sys.inherits(MCCPChannel, Channel);
function MCCPChannel() {
  Channel.call(this);
  
  this.on("open", function(who) {});
  this.on("close", function(who) {});
  this.on("focus", function() {});
  this.on("blur", function() {});
  this.on("data", function(data) {});
}

var nvt = new NVT();
var mccp = new MCCPChannel();

mccp.attach(nvt, 86);
mccp.toggle({remote: true, local: "lazy"});

*/
