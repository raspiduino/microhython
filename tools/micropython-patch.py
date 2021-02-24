'''
Microhython - Python in your hand!
Copyright (c) 2021 @raspiduino
Date created 24/2/2021
----------------------------------
This file apply a small patch on
Micropython's source code.
'''

f = open("micropython/lib/utils/pyexec.c", "a+") # Patch to this file
f.seek(0)
content = f.read() # Read that file
content.replace("int ret = parse_compile_execute(MP_STATE_VM(repl_line), MP_PARSE_SINGLE_INPUT, EXEC_FLAG_ALLOW_DEBUGGING | EXEC_FLAG_IS_REPL | EXEC_FLAG_SOURCE_IS_VSTR);", "if(repl_line != \":UP\" && repl_line != \":DOWN\") {\n            int ret = parse_compile_execute(MP_STATE_VM(repl_line), MP_PARSE_SINGLE_INPUT, EXEC_FLAG_ALLOW_DEBUGGING | EXEC_FLAG_IS_REPL | EXEC_FLAG_SOURCE_IS_VSTR);\n        } else {\n            if(repl_line == \":UP\") {\n                int ret = parse_compile_execute(MP_STATE_VM(\"import microhython_up\"), MP_PARSE_SINGLE_INPUT, EXEC_FLAG_ALLOW_DEBUGGING | EXEC_FLAG_IS_REPL | EXEC_FLAG_SOURCE_IS_VSTR);\n            } else {\n                int ret = parse_compile_execute(MP_STATE_VM(\"import microhython_down\"), MP_PARSE_SINGLE_INPUT, EXEC_FLAG_ALLOW_DEBUGGING | EXEC_FLAG_IS_REPL | EXEC_FLAG_SOURCE_IS_VSTR);\n            }\n")
f.seek(0)
f.write(content)
f.close()
print("Done apply patch!")
