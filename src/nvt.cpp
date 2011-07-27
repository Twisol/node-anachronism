#include <cassert>
#include <node_buffer.h>

#include "nvt.hpp"

using namespace v8;
using namespace node;

Persistent<FunctionTemplate> NVT::constructor_template;

static Persistent<ObjectTemplate> event_template;


bool NVT::HasInstance(Handle<Value> value) {
  return constructor_template->HasInstance(value);
}

void NVT::Init(Handle<Object> target) {
  // Create a function template for this class
  Local<FunctionTemplate> ctor(FunctionTemplate::New(&NVT::New));
  
  ctor->Inherit(EventEmitter::constructor_template);
  
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

  /*
    bool Emit(v8::Handle<v8::String> event,
              int argc,
              v8::Handle<v8::Value> argv[]);
   */

Handle<Value> NVT::New(const Arguments& args) {
  HandleScope scope;
  
  Local<Function> eventCallback = Local<Function>::Cast(args[0]);
  Local<Function> teloptCallback = Local<Function>::Cast(args[1]);
  
  NVT* telnet = new NVT(
    Persistent<Function>::New(eventCallback),
    Persistent<Function>::New(teloptCallback));
  telnet->Wrap(args.This());
  return args.This();
}


NVT::NVT(Persistent<Function> eventCallback, Persistent<Function> teloptCallback) {
  onEvent_ = eventCallback;
  onTeloptEvent_ = teloptCallback;
  nvt_ = telnet_nvt_new(&NVT::OnTelnetEvent, &NVT::OnTeloptEvent, (void*)this);
}

NVT::~NVT() {
  onEvent_.Dispose();
  onTeloptEvent_.Dispose();
  telnet_nvt_free(nvt_);
}

telnet_error NVT::InterruptParser() {
  return telnet_interrupt(this->nvt_);
}

v8::Handle<v8::Value> NVT::InterruptParser(const v8::Arguments& args) {
  NVT* nvt = ObjectWrap::Unwrap<NVT>(args.This());
  nvt->InterruptParser();
  return Undefined();
}


void NVT::OnTelnetEvent(telnet_event* event) {
  HandleScope scope;
  
  Local<Object> obj = ObjectTemplate::New()->NewInstance();
  
  switch (event->type) {
    case TELNET_EV_DATA: {
      telnet_data_event* ev = (telnet_data_event*)event;
      obj->Set(String::NewSymbol("type"), String::NewSymbol("text"));
      obj->Set(String::NewSymbol("text"), Buffer::New((char*)ev->data, ev->length)->handle_);
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
      obj->Set(String::NewSymbol("data"), Buffer::New((char*)ev->data, ev->length)->handle_);
      break;
    }
  }
  
  Local<Value> args[1] = {obj};
  onEvent_->Call(Context::GetCurrent()->Global(), 1, args);
}

void NVT::OnTeloptEvent(telnet_byte telopt, telnet_telopt_event* event) {
  HandleScope scope;
  
  Local<Object> obj = ObjectTemplate::New()->NewInstance();
  
  switch (event->type) {
    case TELNET_EV_TELOPT_TOGGLE: {
      telnet_telopt_toggle_event* ev = (telnet_telopt_toggle_event*)event;
      obj->Set(String::NewSymbol("type"), String::NewSymbol("toggle"));
      obj->Set(String::NewSymbol("where"), String::NewSymbol((ev->where == TELNET_LOCAL) ? "local" : "remote"));
      obj->Set(String::NewSymbol("status"), Boolean::New(ev->status == TELNET_ON));
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
      obj->Set(String::NewSymbol("text"), Buffer::New((char*)ev->data, ev->length)->handle_);
      break;
    }
  }
  
  Local<Value> args[2] = {Integer::NewFromUnsigned(telopt), obj};
  //onTeloptEvent_->Call(Context::GetCurrent()->Global(), 2, args);
}

telnet_error NVT::Receive(telnet_byte* data, size_t length, size_t* bytes_used) {
  return telnet_receive(this->nvt_, data, length, bytes_used);
}

telnet_error NVT::SendText(telnet_byte* data, size_t length) {
  return telnet_send_data(this->nvt_, data, length);
}

telnet_error NVT::SendCommand(telnet_byte command) {
  return telnet_send_command(this->nvt_, command);
}

telnet_error NVT::SendSubnegotiation(telnet_byte telopt, telnet_byte* data, size_t length) {
  return telnet_send_subnegotiation(this->nvt_, telopt, data, length);
}


void NVT::OnTelnetEvent(telnet_nvt* nvt, telnet_event* event) {
  NVT* telnet = NULL;
  telnet_get_userdata(nvt, (void**)&telnet);
  telnet->OnTelnetEvent(event);
}

void NVT::OnTeloptEvent(telnet_nvt* nvt, telnet_byte telopt, telnet_telopt_event* event) {
  NVT* telnet = NULL;
  telnet_get_userdata(nvt, (void**)&telnet);
  telnet->OnTeloptEvent(telopt, event);
}

Handle<Value> NVT::Receive(const Arguments& args) {
  HandleScope scope;
  
  if (!Buffer::HasInstance(args[0])) {
    return ThrowException(Exception::Error(String::New("Argument #1 must be a Buffer.")));
  }
  
  Local<Object> buf = args[0]->ToObject();
  const char* data = Buffer::Data(buf);
  size_t len = Buffer::Length(buf);
  
  NVT* nvt = ObjectWrap::Unwrap<NVT>(args.This());
  size_t bytes_used;
  nvt->Receive((telnet_byte*)data, len, &bytes_used);
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
  
  NVT* nvt = ObjectWrap::Unwrap<NVT>(args.This());
  nvt->SendText((telnet_byte*)data, len);
  return Undefined();
}

v8::Handle<v8::Value> NVT::SendCommand(const v8::Arguments& args) {
  HandleScope scope;
  
  telnet_byte command = (telnet_byte)args[0]->Uint32Value();
  
  NVT* nvt = ObjectWrap::Unwrap<NVT>(args.This());
  nvt->SendCommand(command);
  return Undefined();
}

v8::Handle<v8::Value> NVT::SendSubnegotiation(const v8::Arguments& args) {
  HandleScope scope;
  
  if (!Buffer::HasInstance(args[1])) {
    return ThrowException(Exception::Error(String::New("Argument #1 must be a Buffer.")));
  }
  
  telnet_byte command = (telnet_byte)args[0]->Uint32Value();
  
  Local<Object> buf = args[1]->ToObject();
  const char* data = Buffer::Data(buf);
  size_t len = Buffer::Length(buf);
  
  NVT* nvt = ObjectWrap::Unwrap<NVT>(args.This());
  nvt->SendSubnegotiation(command, (telnet_byte*)data, len);
  return Undefined();
}



telnet_error NVT::EnableLocalTelopt(const telnet_byte telopt, unsigned char lazy) {
  return telnet_telopt_enable_local(this->nvt_, telopt, lazy);
}

v8::Handle<v8::Value> NVT::EnableLocalTelopt(const v8::Arguments& args) {
  HandleScope scope;
  
  telnet_byte command = (telnet_byte)args[0]->Uint32Value();
  unsigned char lazy = (args[1]->BooleanValue()) ? 1 : 0;
  
  NVT* nvt = ObjectWrap::Unwrap<NVT>(args.This());
  nvt->EnableLocalTelopt(command, lazy);
  return Undefined();
}

telnet_error NVT::EnableRemoteTelopt(const telnet_byte telopt, unsigned char lazy) {
  return telnet_telopt_enable_remote(this->nvt_, telopt, lazy);
}

v8::Handle<v8::Value> NVT::EnableRemoteTelopt(const v8::Arguments& args) {
  HandleScope scope;
  
  telnet_byte command = (telnet_byte)args[0]->Uint32Value();
  unsigned char lazy = (args[1]->BooleanValue()) ? 1 : 0;
  
  NVT* nvt = ObjectWrap::Unwrap<NVT>(args.This());
  nvt->EnableRemoteTelopt(command, lazy);
  return Undefined();
}

telnet_error NVT::DisableLocalTelopt(const telnet_byte telopt) {
  return telnet_telopt_disable_local(this->nvt_, telopt);
}

v8::Handle<v8::Value> NVT::DisableLocalTelopt(const v8::Arguments& args) {
  HandleScope scope;
  
  telnet_byte command = (telnet_byte)args[0]->Uint32Value();
  
  NVT* nvt = ObjectWrap::Unwrap<NVT>(args.This());
  nvt->DisableLocalTelopt(command);
  return Undefined();
}

telnet_error NVT::DisableRemoteTelopt(const telnet_byte telopt) {
  return telnet_telopt_disable_remote(this->nvt_, telopt);
}

v8::Handle<v8::Value> NVT::DisableRemoteTelopt(const v8::Arguments& args) {
  HandleScope scope;
  
  telnet_byte command = (telnet_byte)args[0]->Uint32Value();
  
  NVT* nvt = ObjectWrap::Unwrap<NVT>(args.This());
  nvt->DisableLocalTelopt(command);
  return Undefined();
}

telnet_telopt_mode NVT::LocalTeloptStatus(const telnet_byte telopt) {
  telnet_telopt_mode status;
  telnet_telopt_status_local(this->nvt_, telopt, &status);
  return status;
}

v8::Handle<v8::Value> NVT::LocalTeloptStatus(const v8::Arguments& args) {
  HandleScope scope;
  
  telnet_byte command = (telnet_byte)args[0]->Uint32Value();
  
  NVT* nvt = ObjectWrap::Unwrap<NVT>(args.This());
  telnet_telopt_mode status = nvt->LocalTeloptStatus(command);
  return scope.Close(Boolean::New(status == TELNET_ON));
}

telnet_telopt_mode NVT::RemoteTeloptStatus(const telnet_byte telopt) {
  telnet_telopt_mode status;
  telnet_telopt_status_remote(this->nvt_, telopt, &status);
  return status;
}

v8::Handle<v8::Value> NVT::RemoteTeloptStatus(const v8::Arguments& args) {
  HandleScope scope;
  
  telnet_byte command = (telnet_byte)args[0]->Uint32Value();
  
  NVT* nvt = ObjectWrap::Unwrap<NVT>(args.This());
  telnet_telopt_mode status = nvt->RemoteTeloptStatus(command);
  return scope.Close(Boolean::New(status == TELNET_ON));
}
