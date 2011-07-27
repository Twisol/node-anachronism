#include <v8.h>
#include <node.h>

#include "nvt.hpp"

using namespace v8;

extern "C" {
  static void init(Handle<Object> target) {
    NVT::Init(target);
  }
  
  NODE_MODULE(anachronism, init);
}
