#include <cassert>
#include <cstring>
#include <node_buffer.h>

#include "nvt.hpp"

using namespace v8;
using namespace node;

Persistent<FunctionTemplate> NVT::constructor_template;

static Persistent<ObjectTemplate> event_template;

static Local<Object> make_buffer_from_data(const char* data, size_t length) {
  HandleScope scope;
  
  Buffer* buf = Buffer::New(length);
  memcpy(Buffer::Data(buf), data, length);
  
  Local<Object> ctx = Context::GetCurrent()->Global();
  
  Local<Function> buffer_ctor = Local<Function>::Cast(ctx->Get(String::NewSymbol("Buffer")));
  Handle<Value> args[3] = {
    buf->handle_,
    Integer::New(length),
    Integer::New(0),
  };
  return scope.Close(buffer_ctor->NewInstance(3, args));
}


bool NVT::HasInstance(Handle<Value> value) {
  return constructor_template->HasInstance(value);
}

void NVT::Init(Handle<Object> target) {
  // Create a function template for this class
  Local<FunctionTemplate> ctor(FunctionTemplate::New(&NVT::New));
  
  // Make space to store a reference to a NVT instance.
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(String::NewSymbol("NVT"));
  
  // Attach methods to the prototype.
  NODE_SET_PROTOTYPE_METHOD(ctor, "receive", &NVT::Receive);
  NODE_SET_PROTOTYPE_METHOD(ctor, "sendText", &NVT::SendText);
  NODE_SET_PROTOTYPE_METHOD(ctor, "sendCommand", &NVT::SendCommand);
  NODE_SET_PROTOTYPE_METHOD(ctor, "sendSubnegotiation", &NVT::SendSubnegotiation);
  NODE_SET_PROTOTYPE_METHOD(ctor, "interruptParser", &NVT::InterruptParser);
  NODE_SET_PROTOTYPE_METHOD(ctor, "requestEnableLocal", &NVT::EnableLocalTelopt);
  NODE_SET_PROTOTYPE_METHOD(ctor, "requestEnableRemote", &NVT::EnableRemoteTelopt);
  NODE_SET_PROTOTYPE_METHOD(ctor, "requestDisableLocal", &NVT::DisableLocalTelopt);
  NODE_SET_PROTOTYPE_METHOD(ctor, "requestDisableRemote", &NVT::DisableRemoteTelopt);
  NODE_SET_PROTOTYPE_METHOD(ctor, "requestStatusLocal", &NVT::LocalTeloptStatus);
  NODE_SET_PROTOTYPE_METHOD(ctor, "requestStatusRemote", &NVT::RemoteTeloptStatus);
  
  // Keep the function template from getting GC'd
  NVT::constructor_template = Persistent<FunctionTemplate>::New(ctor);
  
  // Attach the constructor function to the namespace provided.
  target->Set(String::NewSymbol("Telnet"), ctor->GetFunction());
  
  
  // Create the template for event objects
  event_template = Persistent<ObjectTemplate>(ObjectTemplate::New());
}

Handle<Value> NVT::New(const Arguments& args) {
  HandleScope scope;
  
  Local<Function> eventCallback = Local<Function>::Cast(args[0]);
  Local<Function> teloptCallback = Local<Function>::Cast(args[1]);
  Local<Function> negotiateCallback = Local<Function>::Cast(args[2]);
  
  NVT* self = new NVT();
  self->onEvent_ = Persistent<Function>::New(eventCallback);
  self->onTeloptEvent_ = Persistent<Function>::New(teloptCallback);
  self->onNegotiateEvent_ = Persistent<Function>::New(negotiateCallback);
  
  // TODO: pass a negotiate callback to telnet_nvt_new().
  self->nvt_ = telnet_nvt_new((void*)self, &OnTelnetEvent, &OnTeloptEvent, &OnNegotiateEvent);
  
  self->Wrap(args.This());
  return args.This();
}

NVT::~NVT() {
  onEvent_.Dispose();
  onTeloptEvent_.Dispose();
  telnet_nvt_free(nvt_);
}

v8::Handle<v8::Value> NVT::InterruptParser(const v8::Arguments& args) {
  NVT* self = ObjectWrap::Unwrap<NVT>(args.This());
  telnet_interrupt(self->nvt_);
  return Undefined();
}

void NVT::OnTelnetEvent(telnet_nvt* nvt, telnet_event* event) {
  HandleScope scope;
  
  NVT* self = NULL;
  telnet_get_userdata(nvt, (void**)&self);
  
  Local<Object> obj = ObjectTemplate::New()->NewInstance();
  
  switch (event->type) {
    case TELNET_EV_DATA: {
      telnet_data_event* ev = (telnet_data_event*)event;
      obj->Set(String::NewSymbol("type"), String::NewSymbol("text"));
      obj->Set(String::NewSymbol("text"), make_buffer_from_data((const char*)ev->data, ev->length));
      break;
    }
    case TELNET_EV_COMMAND: {
      telnet_command_event* ev = (telnet_command_event*)event;
      obj->Set(String::NewSymbol("type"), String::NewSymbol("command"));
      obj->Set(String::NewSymbol("command"), Integer::NewFromUnsigned(ev->command));
      break;
    }
    case TELNET_EV_WARNING: {
      telnet_warning_event* ev = (telnet_warning_event*)event;
      obj->Set(String::NewSymbol("type"), String::NewSymbol("warning"));
      obj->Set(String::NewSymbol("message"), String::New((char*)ev->message));
      obj->Set(String::NewSymbol("position"), Integer::NewFromUnsigned(ev->position));
      break;
    }
    case TELNET_EV_SEND: {
      telnet_send_event* ev = (telnet_send_event*)event;
      obj->Set(String::NewSymbol("type"), String::NewSymbol("send"));
      obj->Set(String::NewSymbol("data"), make_buffer_from_data((const char*)ev->data, ev->length));
      break;
    }
  }
  
  Local<Value> args[1] = {obj};
  self->onEvent_->Call(Context::GetCurrent()->Global(), 1, args);
}

void NVT::OnTeloptEvent(telnet_nvt* nvt, telnet_byte telopt, telnet_telopt_event* event) {
  HandleScope scope;
  
  NVT* self = NULL;
  telnet_get_userdata(nvt, (void**)&self);
  
  Local<Object> obj = ObjectTemplate::New()->NewInstance();
  
  switch (event->type) {
    case TELNET_EV_TELOPT_TOGGLE: {
      telnet_telopt_toggle_event* ev = (telnet_telopt_toggle_event*)event;
      obj->Set(String::NewSymbol("type"), String::NewSymbol("toggle"));
      obj->Set(String::NewSymbol("where"), String::NewSymbol((ev->where == TELNET_LOCAL) ? "local" : "remote"));
      obj->Set(String::NewSymbol("status"), Boolean::New(ev->status));
      break;
    }
    case TELNET_EV_TELOPT_FOCUS: {
      telnet_telopt_focus_event* ev = (telnet_telopt_focus_event*)event;
      obj->Set(String::NewSymbol("type"), String::NewSymbol("focus"));
      obj->Set(String::NewSymbol("focus"), Boolean::New(ev->focus));
      break;
    }
    case TELNET_EV_TELOPT_DATA: {
      telnet_telopt_data_event* ev = (telnet_telopt_data_event*)event;
      obj->Set(String::NewSymbol("type"), String::NewSymbol("text"));
      obj->Set(String::NewSymbol("text"), make_buffer_from_data((const char*)ev->data, ev->length));
      break;
    }
  }
  
  Local<Value> args[2] = {Integer::NewFromUnsigned(telopt), obj};
  self->onTeloptEvent_->Call(Context::GetCurrent()->Global(), 2, args);
}

unsigned char NVT::OnNegotiateEvent(telnet_nvt* nvt, telnet_byte telopt, telnet_telopt_location where) {
  HandleScope scope;
  
  NVT* self = NULL;
  telnet_get_userdata(nvt, (void**)&self);
  
  Local<Value> args[2] = {
    Integer::NewFromUnsigned(telopt),
    String::NewSymbol((where == TELNET_LOCAL) ? "locaL" : "remote"),
  };
  
  Local<Value> result = self->onNegotiateEvent_->Call(Context::GetCurrent()->Global(), 2, args);
  return result->ToBoolean()->Value();
}

Handle<Value> NVT::Receive(const Arguments& args) {
  HandleScope scope;
  
  if (!Buffer::HasInstance(args[0])) {
    return ThrowException(Exception::Error(String::New("Argument #1 must be a Buffer.")));
  }
  
  Local<Object> buf = args[0]->ToObject();
  const char* data = Buffer::Data(buf);
  size_t len = Buffer::Length(buf);
  
  NVT* self = ObjectWrap::Unwrap<NVT>(args.This());
  
  size_t bytes_used;
  telnet_receive(self->nvt_, (telnet_byte*)data, len, &bytes_used);
  
  return scope.Close(Integer::New(bytes_used));
}

Handle<Value> NVT::SendText(const Arguments& args) {
  HandleScope scope;
  
  if (!Buffer::HasInstance(args[0])) {
    return ThrowException(Exception::Error(String::New("Argument #1 must be a Buffer.")));
  }
  
  Local<Object> buf = args[0]->ToObject();
  const char* data = Buffer::Data(buf);
  size_t len = Buffer::Length(buf);
  
  NVT* self = ObjectWrap::Unwrap<NVT>(args.This());
  telnet_send_data(self->nvt_, (telnet_byte*)data, len);
  return Undefined();
}

v8::Handle<v8::Value> NVT::SendCommand(const v8::Arguments& args) {
  HandleScope scope;
  
  telnet_byte command = (telnet_byte)args[0]->Uint32Value();
  
  NVT* self = ObjectWrap::Unwrap<NVT>(args.This());
  telnet_send_command(self->nvt_, command);
  return Undefined();
}

v8::Handle<v8::Value> NVT::SendSubnegotiation(const v8::Arguments& args) {
  HandleScope scope;
  
  if (!Buffer::HasInstance(args[1])) {
    return ThrowException(Exception::Error(String::New("Argument #1 must be a Buffer.")));
  }
  
  telnet_byte telopt = (telnet_byte)args[0]->Uint32Value();
  
  Local<Object> buf = args[1]->ToObject();
  const char* data = Buffer::Data(buf);
  size_t len = Buffer::Length(buf);
  
  NVT* self = ObjectWrap::Unwrap<NVT>(args.This());
  telnet_send_subnegotiation(self->nvt_, telopt, (telnet_byte*)data, len);
  return Undefined();
}


v8::Handle<v8::Value> NVT::EnableLocalTelopt(const v8::Arguments& args) {
  HandleScope scope;
  
  telnet_byte telopt = (telnet_byte)args[0]->Uint32Value();
  
  NVT* self = ObjectWrap::Unwrap<NVT>(args.This());
  telnet_telopt_enable(self->nvt_, telopt, TELNET_LOCAL);
  return Undefined();
}

v8::Handle<v8::Value> NVT::EnableRemoteTelopt(const v8::Arguments& args) {
  HandleScope scope;
  
  telnet_byte telopt = (telnet_byte)args[0]->Uint32Value();
  
  NVT* self = ObjectWrap::Unwrap<NVT>(args.This());
  telnet_telopt_enable(self->nvt_, telopt, TELNET_REMOTE);
  return Undefined();
}

v8::Handle<v8::Value> NVT::DisableLocalTelopt(const v8::Arguments& args) {
  HandleScope scope;
  
  telnet_byte telopt = (telnet_byte)args[0]->Uint32Value();
  
  NVT* self = ObjectWrap::Unwrap<NVT>(args.This());
  telnet_telopt_disable(self->nvt_, telopt, TELNET_LOCAL);
  return Undefined();
}

v8::Handle<v8::Value> NVT::DisableRemoteTelopt(const v8::Arguments& args) {
  HandleScope scope;
  
  telnet_byte telopt = (telnet_byte)args[0]->Uint32Value();
  
  NVT* self = ObjectWrap::Unwrap<NVT>(args.This());
  telnet_telopt_disable(self->nvt_, telopt, TELNET_REMOTE);
  return Undefined();
}

v8::Handle<v8::Value> NVT::LocalTeloptStatus(const v8::Arguments& args) {
  HandleScope scope;
  
  telnet_byte telopt = (telnet_byte)args[0]->Uint32Value();
  
  NVT* self = ObjectWrap::Unwrap<NVT>(args.This());
  unsigned char status;
  telnet_telopt_status(self->nvt_, telopt, TELNET_LOCAL, &status);
  return scope.Close(Boolean::New(status));
}

v8::Handle<v8::Value> NVT::RemoteTeloptStatus(const v8::Arguments& args) {
  HandleScope scope;
  
  telnet_byte telopt = (telnet_byte)args[0]->Uint32Value();
  
  NVT* self = ObjectWrap::Unwrap<NVT>(args.This());
  unsigned char status;
  telnet_telopt_status(self->nvt_, telopt, TELNET_REMOTE, &status);
  return scope.Close(Boolean::New(status));
}
