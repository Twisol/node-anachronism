from os import system

def set_options(opt):
  opt.tool_options("compiler_cxx")
  
def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  
def build(bld):
  system("cd vendor/anachronism/ && make")
  
  obj = bld.new_task_gen("cxx", "shlib", "node_addon") 
  obj.cxxflags = ["-g", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall", "-I../vendor/anachronism/include"]
  obj.linkflags = ['-L../vendor/anachronism/build/', '-lanachronism']
  
  obj.target = "anachronism"
  obj.source = "src/anachronism.cpp src/nvt.cpp"
