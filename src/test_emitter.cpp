#include "test_emitter.hpp"

#include <system_error>
#include <pstream.h>
#include <regex>

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

std::pair<std::string, std::string> parseErrorMsg(std::string msg)
{
        std::string m_var1 = "", m_var2 = "", vars = "";
        std::pair<std::string, std::string> result("","");

        std::regex r1("Assertion `r_[0-9]+.[a-zA-Z0-9_=.]+\\(r_[0-9]+\\)' failed");
        std::regex r2("r_[0-9]+");

        std::smatch m1, m2;

        std::regex_search(msg, m1, r1);

//      std::cout << "Message: " << msg << " : " << m1.size() << " : " << m1.str(0) << std::endl;

        int count = 1;

        #if 1
        if (m1.size() == 1)
        {
                vars = m1.str(0);

                try
                {
                        std::sregex_iterator next(vars.begin(), vars.end(), r2);
                        std::sregex_iterator end;

                        while (next != end)
                        {
                                std::smatch match = *next;

                                if(count == 1)
                                {
                                        m_var1 = match.str();
                                }
                                else if(count == 2)
                                {
                                        m_var2 = match.str();
                                }

                                count++;
                                next++;
                        } 
                } catch (std::regex_error& e) {
                        // Syntax error in the regular expression
                }
                
//              std::cout << "Var1: " << m_var1 << std::endl;
//              std::cout << "Var2: " << m_var2 << std::endl;
        } 
        #endif

        result = std::make_pair(m_var1, m_var2);
	
	return result;
}

std::string Exec(const char* cmd)
{
	std::string err = "";

//	redi::ipstream proc(cmd, redi::pstreams::pstderr);
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

bool isAlertError(std::string exe_err)
{
	std::string var1 = "";	
	std::string var2 = "";	

	std::string delim = ",";
        auto start = 0U;
        auto end = exe_err.find(delim);

        int count = 1;

        while (end != std::string::npos)
        {
                if(count == 1)
                {
                        var1 = exe_err.substr(start, end - start);
                }
                else if(count == 2)
                {
                        var2 = exe_err.substr(start, end - start);
                }

                count++;

                start = end + delim.length();
                end = exe_err.find(delim, start);
        }

        var2 = exe_err.substr(start, end);

	if(var1 == "" || var2 == "")
	{
		return false;
	}

	return true;
}

std::string exeExec(const char* cmd)
{
        std::string err = "";

        redi::ipstream proc(cmd, redi::pstreams::pstdout | redi::pstreams::pstderr);
        std::string line, last_line;

	last_line = "";

        // read child's stdout
//      while (std::getline(proc.out(), line))
//      std::cout << "stdout: " << line << '\n';

        std::size_t pos1;

        // read child's stderr
        while (std::getline(proc.err(), line))
        {
//              std::cout << "stderr: " << line << '\n';
		last_line = line;

                pos1 = line.find("Assertion `r");  // Assumption that the error caused due to Assertion failure caused by meta variants

                if (pos1 != std::string::npos)
                        err += line;
        }

//      std::cout << "Err: " << err << std::endl;

        std::pair<std::string, std::string> res;

	if(err != "")
	{
		res = parseErrorMsg(err);
        	std::string new_err = "";

        	if(res.first != "")
        	{
                	new_err = res.first + "," + res.second;
        	}

//        	std::cout << "New Err: " << new_err << std::endl;
        	return new_err;
	}
	else
	{
		return last_line;
	}
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

//    api_fuzzer->tree.traverse();

    #if REDUCE 
    // Code Added by Pritam

    std::string compile_bin = config_data["meta_runner"]["test_compile_bin"].as<std::string>();
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

    std::string compile_cmd = compile_dir + "/" + compile_bin + " " + args.output_file;

    command = compile_cmd.c_str();

   // Executing the Test Case

    std::string execute_cmd = compile_dir + "/" + testobj;
       	
    exe_command = execute_cmd.c_str();

    std::string compile_err, exe_err;

    std::pair<std::string, std::string> res;	

//    std::cout << "Exe: " << exe_command << std::endl;

    compile_err = Exec(command);
    exe_err = exeExec(exe_command);

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

//		res = parseErrorMsg(exe_err);

		if(exe_err != "") // Execution failed but not because of the assertion failure
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

			std::vector<const ApiInstructionInterface*> red = api_fuzzer->MHReduceInstrPrep(compile_err, exe_err, var, args.output_file);

//			std::cout << "Instructions red: " << red.size() << std::endl;
//			printVectorApiInstructions(red);

			std::vector<const ApiInstructionInterface*> input_insts;

			input_insts = api_fuzzer->fuzzerReduction(compile_err, exe_err, args.output_file, red);

			input_insts = api_fuzzer->reduceSubTree(compile_err, exe_err, args.output_file, red);

			red = api_fuzzer->replaceMetaInputVariables(compile_err, exe_err, red, args.output_file);

//			std::cout << "Instructions after Fuzzing: " << input_insts.size() << std::endl;
//			printVectorApiInstructions(input_insts);

			api_fuzzer->simplifyMetaRelationsPrep(compile_err, exe_err, var, args.output_file, api_fuzzer->tree.traverse(), red);
		}
		else
                {
                        std::cout << "Not an assertion failure" << std::endl;
                }
	  }	
      }	
    #endif
}
