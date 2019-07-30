#include "test_emitter.hpp"

#include <system_error>
#include <pstream.h>

#define REDUCE 1

static unsigned int indent = 0;
const std::string default_config_file =
    "/home/sentenced/Documents/Internships/2018_ETH/work/sets/config_files/config_isl.yaml";

bool DEBUG = false;
bool META_TESTING = true;

std::map<std::string, Modes> string_to_mode {
    {"SET_FUZZ", SET_FUZZ},
    {"API_FUZZ", API_FUZZ},
    {"SET_TEST", SET_TEST},
    {"SET_META_STR", SET_META_STR},
    {"SET_META_API", SET_META_API},
    {"SET_META_NEW", SET_META_NEW},
};

std::stringstream new_ss_i, new_ss_mi, new_ss_p;
const char *command, *exe_command;

void
parseArgs(Arguments& args, int argc, char **argv)
{
    int i = 1;
    while (i < argc)
    {
        /* Arguments with options required */
        if (!strcmp(argv[i], "--seed") || !strcmp(argv[i], "-s"))
        {
            args.seed = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "--config-file") || !strcmp(argv[i], "-c"))
        {
            args.config_file = argv[++i];
        }
        else if (!strcmp(argv[i], "--output-file") || !strcmp(argv[i], "-o"))
        {
            args.output_file = argv[++i];
        }
        /* Flag arguments */
        else if (!strcmp(argv[i], "--debug"))
        {
            DEBUG = true;
        }
        else {
            std::cout << "Found unknown argument: " << argv[i] << std::endl;
            exit(1);
        }
        i++;
    }
}

YAML::Node
loadYAMLFileWithCheck(const std::string& file_path)
{
    try
    {
        return YAML::LoadFile(file_path);
    }
    catch (const YAML::BadFile& e)
    {
        std::cout << "Failed loading YAML file at path " << file_path << std::endl;
        exit(1);
    }
}

void
writeLine(std::stringstream &ss, std::string line)
{
    int indent_count = 0;
    while (indent_count++ < indent)
        ss << "\t";
    ss << line << std::endl;
}

void
prepareHeader(std::stringstream &ss, std::vector<std::string> &include_list,
    Arguments& args, std::string api_file, std::string meta_file)
{
    writeLine(ss, fmt::format("// SEED : {}", args.seed));
    writeLine(ss, fmt::format("// API CONFIG FILE : {}", api_file));
    writeLine(ss, fmt::format("// META CONFIG FILE : {}", meta_file));
    std::time_t curr_time_t = std::time(nullptr);
    writeLine(ss, fmt::format("// GENERATION TIME : {}",
        std::ctime(&curr_time_t)));
    writeLine(ss, "");
    for (std::string incl : include_list)
        writeLine(ss, "#include " + incl);
}

void
mainPreSetup(std::stringstream &ss, std::vector<std::string>& pre_setup_instrs)
{
    writeLine(ss, "int main()");
    writeLine(ss, "{");
    indent++;
    for (std::string pre_setup_instr : pre_setup_instrs)
    {
        writeLine(ss, pre_setup_instr);
    }
}

void
mainPostSetup(std::stringstream &ss)
{
    indent--;
    writeLine(ss, "}");
}

bool parseAssertInstruction(std::set<std::string> var, const ApiInstructionInterface *inst)
{
	return true;
}

std::pair<std::string, std::string> parseErrorMsg(std::string msg)
{
	std::string m_var1, m_var2;
	std::pair<std::string, std::string> result("","");

	std::size_t pos1 = msg.find("Assertion `r");  // Assumption that the error caused due to Assertion failure caused by meta variants
	std::size_t pos2, pos3, pos4;

	if (pos1 != std::string::npos)
	{
		pos2 = msg.find(".operator");  
		pos3 = msg.find("==(");  
		pos4 = msg.find(")'");  

		m_var1 = msg.substr(pos1+11,pos2-pos1-11); 

		m_var2 = msg.substr(pos3+3,pos4-pos3-3); 

//		std::cout << "Var1: " << m_var1 << std::endl;
//		std::cout << "Var2: " << m_var2 << std::endl;
	
		result = std::make_pair(m_var1, m_var2);
	} 

	return result;
}

std::string Exec(const char* cmd)
{
	std::string err = "";

	redi::ipstream proc(cmd, redi::pstreams::pstdout | redi::pstreams::pstderr);
	std::string line;

	// read child's stdout
//	while (std::getline(proc.out(), line))
//	std::cout << "stdout: " << line << '\n';

	// read child's stderr
	while (std::getline(proc.err(), line))
	{
//		std::cout << "stderr: " << line << '\n';
		err += line;
	}

	return err;
}

int
main(int argc, char** argv)
{
    Arguments args;
    parseArgs(args, argc, argv);

    if (args.config_file.empty())
    {
        args.config_file = default_config_file;
    }

    YAML::Node config_data = loadYAMLFileWithCheck(args.config_file);
    std::string working_dir = config_data["working_dir"].as<std::string>();
    std::string api_fuzzer_path =
        working_dir + config_data["api_fuzzer_file"].as<std::string>();
    std::string meta_test_path =
        working_dir + config_data["meta_test_file"].as<std::string>();
    if (args.output_file.empty())
    {
        args.output_file =
            working_dir + config_data["test_emitter_output_file"]
                .as<std::string>();
    }

    std::mt19937* rng = new std::mt19937(args.seed);
    std::stringstream new_ss_m;

    YAML::Node api_fuzzer_data = loadYAMLFileWithCheck(api_fuzzer_path);
    std::vector<std::string> include_list = {
        "<cassert>",
        "<iostream>",
    };
    if (api_fuzzer_data["includes"].IsDefined())
    {
        for (YAML::Node include_yaml : api_fuzzer_data["includes"])
        {
            include_list.push_back(fmt::format("\"{}\"",
                include_yaml.as<std::string>()));
        }
    }

    prepareHeader(new_ss_i, include_list, args, api_fuzzer_path, meta_test_path);
    std::vector<std::string> pre_setup_instrs;
    if (api_fuzzer_data["pre_setup"].IsDefined())
    {
        for (YAML::Node pre_setup_yaml : api_fuzzer_data["pre_setup"])
        {
            pre_setup_instrs.push_back(pre_setup_yaml.as<std::string>());
        }
    }
    mainPreSetup(new_ss_i, pre_setup_instrs);

    std::unique_ptr<ApiFuzzerNew> api_fuzzer (
        new ApiFuzzerNew(api_fuzzer_path, meta_test_path, args.seed, rng));
    for (std::string instr : api_fuzzer->getInstrStrs())
    {
        writeLine(new_ss_m, instr);
    }

    mainPostSetup(new_ss_p);

    std::ofstream ofs;
    ofs.open(args.output_file);
    ofs << new_ss_i.str();
    ofs << new_ss_m.str();
    ofs << new_ss_p.str();
    ofs.close();

    #if REDUCE 
    // Code Added by Pritam

    std::string compile_bin = working_dir + config_data["meta_runner"]["test_compile_bin"].as<std::string>();
    std::string compile_dir = working_dir + config_data["meta_runner"]["test_compile_dir"].as<std::string>();

   // Extracting the test object for execution using string manipulations
 
    std::string delim = "/";

    auto start = 0U;
    auto end = args.output_file.find(delim);
    while (end != std::string::npos)
    {
        start = end + delim.length();
        end = args.output_file.find(delim, start);
    }

    std::string testcase = args.output_file.substr(start, end);	
 
    delim = ".";

    start = 0U;
    end = testcase.find(delim);

    std::string testobj = testcase.substr(start, end);	

   // Compiling the Test Case

    std::string compile_cmd = compile_bin + " " + args.output_file;

    command = compile_cmd.c_str();

   // Executing the Test Case

    std::string execute_cmd = compile_dir + "/" + testobj;
       	
    exe_command = execute_cmd.c_str();

    std::string compile_err, exe_err;

    std::pair<std::string, std::string> res;	

//    std::cout << "Exe: " << exe_command << std::endl;

    compile_err = Exec(command);
    exe_err = Exec(exe_command);

//    std::cout << "Compile Error: " << compile_err << std::endl;
//    std::cout << "Execution Error: " << exe_err << std::endl;

    if(compile_err == "" && exe_err == "")
    {
        std::cout << "No Error\n";
    }			
    else	
    {
	if(compile_err != "")
	{
	        std::cout << "Compile Error: " << compile_err << std::endl;
	}
	else if(compile_err == "" && exe_err != "") //Compilation is successful and execution fails
	{
	        std::cout << "Execution Error: " << exe_err << std::endl;

		std::vector<const ApiInstructionInterface*> list_inst, input_inst;

		res = parseErrorMsg(exe_err);

		if(res.first != "") // Execution failed but not because of the assertion failure
		{
			input_inst = api_fuzzer->InputInstrs;
    
			for(std::vector<const ApiInstructionInterface*>::iterator it = input_inst.begin(); it != input_inst.end(); it++)
			{
				writeLine(new_ss_mi, (*it)->toStr());
			}

			// Reducing number of meta variants

			std::vector<const ApiObject*> var;

			var = api_fuzzer->verticalReduction(compile_err, exe_err, api_fuzzer->meta_variants, args.output_file);

			// Reducing number of meta relations 

			std::vector<const ApiInstructionInterface*> red = api_fuzzer->MHReduceInstr(compile_err, res, var, args.output_file);

			#if 0
			for(std::vector<const ApiInstructionInterface*>::iterator it = red.begin(); it != red.end(); it++)
			{
				std::cout << (*it)->toStr() << std::endl;
			}
			#endif

			api_fuzzer->inputVarReduction(compile_err, exe_err, api_func_objects, args.output_file, red);

		}
	  }	
      }	
    #endif
}

void printVectorApiObjects(std::vector<const ApiObject*> var)
{
//	std::cout << "Printing Vector of ApiObjects: " << var.size() << std::endl;
	logDebug(fmt::format("Printing Vector of ApiObjects: {}", var.size()));

	for(std::vector<const ApiObject*>::iterator it = var.begin(); it != var.end(); it++)
	{
//		std::cout << (*it)->toStr() << std::endl;
	        logDebug(fmt::format("{}", (*it)->toStr()));
	}
}

void printVectorApiFuncObjects(std::vector<const ApiFuncObject*> var)
{
//	std::cout << "Printing Vector of ApiFuncObjects: " << var.size() << std::endl;
	logDebug(fmt::format("Printing Vector of ApiFuncObjects: {}", var.size()));

	for(std::vector<const ApiFuncObject*>::iterator it = var.begin(); it != var.end(); it++)
	{
//		std::cout << (*it)->toStr() << std::endl;
	        logDebug(fmt::format("{}", (*it)->toStr()));
	}

//	std::cout << "End of Vector of ApiFuncObjects" << std::endl;
}

void printVectorApiInstructions(std::vector<const ApiInstructionInterface*> instr)
{
//	std::cout << "Printing Vector of ApiInstructions: " << instr.size() << std::endl;
	logDebug(fmt::format("Printing Vector of ApiInstructions: {}", instr.size()));

	for(std::vector<const ApiInstructionInterface*>::iterator it = instr.begin(); it != instr.end(); it++)
	{
//		std::cout << (*it)->toStr() << std::endl;
	        logDebug(fmt::format("{}", (*it)->toStr()));
	}
}
