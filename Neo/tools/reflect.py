import os
import re
import sys

supported_types = [ "u8", "u16", "u32", "u64", "i8", "i16", "i32", "i64", "f32", "f64", 'vec2', 'vec3', 'vec4', 'ivec2', 'ivec3', 'ivec4' ]

def parse_enum(i, lines, out_header, out_body, out_code):
	# TODO - process any directives on the reflect line
	#tokens = lines[i].split()
	#attr_prefix = ""
	#for x, token in enumerate(tokens):
	#		if token == "PREFIX":
	#		attr_prefix = tokens[x+1] + '_'
	#		print(f"found prefix token {attr_prefix}\n")

	tokens = lines[i+1].split()
	enum_name = tokens[1].strip()

	out_header.write(f"enum {enum_name};\n")
	out_header.write(f"bool {enum_name}_StringToEnum(string name, {enum_name} &value);\n")
	out_header.write(f"bool {enum_name}_EnumToString({enum_name} value, string &name);\n\n")

	out_body.write(f"ReflectEnumInfo enumReflect_{enum_name};\n")

	out_body.write(f"bool {enum_name}_StringToEnum(string name, {enum_name} &value)\n")
	out_body.write( "{\n")
	out_body.write(f"  auto it = enumReflect_{enum_name}.map_stringToInt.find(name);\n")
	out_body.write(f"  if (it != enumReflect_{enum_name}.map_stringToInt.end())\n")
	out_body.write( "  {\n")
	out_body.write(f"    value = ({enum_name})it->second;")
	out_body.write( "    return true;\n")
	out_body.write( "  }\n")
	out_body.write( "  return false;\n")
	out_body.write( "}\n")

	out_body.write(f"bool {enum_name}_EnumToString({enum_name} value, string &name)\n")
	out_body.write( "{\n")
	out_body.write(f"  auto it = enumReflect_{enum_name}.map_intToString.find((int)value);\n")
	out_body.write(f"  if (it != enumReflect_{enum_name}.map_intToString.end())\n")
	out_body.write( "  {\n")
	out_body.write( "    name = it->second;\n")
	out_body.write( "    return true;\n")
	out_body.write( "  }\n")
	out_body.write( "  return false;\n")
	out_body.write( "}\n\n")

	out_code.append(f"  enumReflect_{enum_name}.name=\"{enum_name}\";\n")

	i+=2
	ival=0
	sval=""
	while i < len(lines):
		enumTokens = lines[i].split()
		if len(enumTokens) > 0 and enumTokens[0].startswith('}'):
			return i+1
		elif len(enumTokens) > 0 and not enumTokens[0].startswith("\\") and not enumTokens[0].startswith("{"):
			value = enumTokens[0]
			underscoreIndex = value.find('_')
			if underscoreIndex != -1:
				value = value[underscoreIndex+1:]
			value = value.rstrip(',')
			if len(enumTokens) > 2 and enumTokens[1] == "=":
				sval = enumTokens[2].rstrip(',')
				ival=0
			visual_val = ival
			if sval != "":
				visual_val = f"{sval} + {ival}"

			out_code.append(f"  enumReflect_{enum_name}.map_intToString.insert({{ {visual_val},\"{value}\" }});\n")
			out_code.append(f"  enumReflect_{enum_name}.map_stringToInt.insert({{ \"{value}\",{visual_val} }});\n")
			i += 1
			ival += 1
		# just an empty line or an open bracket
		else:
			i += 1
	return i

def parse_struct(i, lines, out_header, out_body, out_code):
	# TODO - process any directives on the reflect line
	i+=1
	tokens = lines[i].split()
	if len(tokens) < 1:
		print("ERROR - struct had no name\n")
		return i
	struct_name = tokens[1]
	out_header.write(f"//#### STRUCT {struct_name} ####\n")
	out_header.write(f"extern ReflectStructInfo reflectStructInfo_{struct_name};\n")
	out_body.write(f"ReflectStructInfo reflectStructInfo_{struct_name}\n{{\n  \"{struct_name}\", sizeof({struct_name}),\n  {{")

	memberIdx = 0
	while i < len(lines):
		tokens = lines[i].split()
		if len(tokens)>0:
			if tokens[0] in supported_types:
				var_type = tokens[0]
				var_name = tokens[1].rstrip(';')

				# terminate the last member with a comma if this isn't the first member
				if memberIdx != 0:
					out_body.write(",")

				out_body.write(f"\n    {{ \"{var_name}\", VarType_{var_type}, sizeof({var_type}), offsetof({struct_name}, {var_name}) }}")
				i += 1
				memberIdx += 1
			elif tokens[0] == "void":

				# terminate the last member with a comma if this isn't the first member
				if memberIdx != 0:
					out_body.write(",")

				func_name = tokens[1].split('(')[0].strip()
				out_body.write(f"\n    {{ \"{func_name}\", VarType_func, 0, 0, [](void* obj) {{ (({struct_name}*)obj)->{func_name}(); }} }}")
				i += 1
				memberIdx += 1
			elif tokens[0].startswith('}'):
				out_body.write("\n  }\n};\n")
				return i+1
			else:
				i += 1
		else:
			i += 1
	print("Error - structure not closed!\n")
	return i

def parse_reflect(i, lines, out_header, out_body, out_code):
	line = lines[i+1].strip()
	if line.startswith("enum"):
		return parse_enum(i, lines, out_header, out_body, out_code)
	elif line.startswith("struct") or line.startswith("class"):
		return parse_struct(i, lines, out_header, out_body, out_code)
	else:
		print("REFLECT is not followed by an enum, struct or class...")
	return i+1

def parse_enums_and_structs(init_func, output_file, list_of_files):
	output_header = output_file + ".h"
	output_body = output_file + ".cpp"
	out_code = []

	with open(output_header, 'w') as out_header, open(output_body, "w") as out_body:
		out_header.write("#pragma once\n")
		out_header.write(f"void {init_func}();\n\n")
		out_body.write("#include \"Neo.h\"\n")
		out_body.write(f"#include \"{output_file}.h\"\n\n")

		for file_path in list_of_files:
			idx = file_path.rfind('\\')
			if idx != -1:
				filename = file_path[idx+1:]
			else:
				filename = file_path
			found_reflect = 0
			with open(file_path, 'r') as f:
				lines = f.readlines()
				i=0
				while i < len(lines):
					line = lines[i].strip()
					if line.startswith("//<REFLECT>"):
						if not found_reflect:
							print(f"found reflect in: {filename}")
							out_body.write(f"#include \"{filename}\"\n")
							found_reflect = 1
						i = parse_reflect(i, lines, out_header, out_body, out_code)
					else:
						i += 1
		out_body.write(f"void {init_func}()\n")
		out_body.write("{\n")
		for item in out_code:
			out_body.write(item)
		out_body.write("}\n")

# Example usage
if len(sys.argv) < 4:
	print("Usage: python reflect.py <initFunc> <outfile> <directories>...")
	sys.exit(1)

list_of_dirs = []
initfunc = sys.argv[1]
outputfile = sys.argv[2]
for outputDir in sys.argv[3:]:
	list_of_dirs.append(outputDir)

list_of_files = []
for dir in list_of_dirs:
	for filename in os.listdir(dir):
		if filename.endswith(".h"):
			file_path = os.path.join(dir, filename)
			list_of_files.append(file_path)

print(f"Scanning {len(list_of_files)} files for reflection data...\n")

dirty = True
if not os.path.exists(outputfile + ".h") or not os.path.exists(outputfile + ".cpp"):
	dirty = True
else:
	out_tm = os.path.getmtime(outputfile + ".h")
	out_body_tm = os.path.getmtime(outputfile + ".cpp")
	if (out_body_tm > out_tm):
		out_tm = out_body_tm
	for file_path in list_of_files:
		filename_tm = os.path.getmtime(file_path)
		if (filename_tm > out_tm):
			dirty = True
			break
if dirty:
	parse_enums_and_structs(initfunc, outputfile, list_of_files)
else:
	print("No file changes detected!")
