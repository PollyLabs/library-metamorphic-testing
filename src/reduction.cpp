#include "reduction.hpp"

int try_outs = 0;


std::vector<const ApiObject*> Reduction::verticalReductionMerge(std::vector<const ApiObject*> mvar1, std::vector<const ApiObject*> mvar2)
{
	for(std::vector<const ApiObject*>::iterator it = mvar2.begin(); it != mvar2.end(); it++)
	{
		if(find(mvar1.begin(), mvar1.end(), *it) == mvar1.end())
		{
			mvar1.push_back(*it);
		}
	}

	return mvar1;
}

std::vector<const ApiInstructionInterface*> Reduction::instructionMerge(std::vector<const ApiInstructionInterface*> mvar1, std::vector<const ApiInstructionInterface*> mvar2)
{
	for(std::vector<const ApiInstructionInterface*>::iterator it = mvar2.begin(); it != mvar2.end(); it++)
	{
		if(find(mvar1.begin(), mvar1.end(), *it) == mvar1.end())
		{
			mvar1.push_back(*it);
		}
	}

	return mvar1;
}

std::vector<const ApiObject*> Reduction::verticalReduction(std::string compile_err, std::string exe_err, std::vector<const ApiObject*> mvar, std::string output_file)
{
	std::vector<const ApiObject*> new_mvar1, new_mvar2, ip1, ip2, new_mvar;

    	logDebug(fmt::format("Inside verticalReduction"));
	printVectorApiObjects(mvar);

	if(mvar.size() <= 1)
	{
		createTestCase(mvar, output_file);
		return mvar;
	}

	//Find the mid of the input vector mvar

	int size = mvar.size();
	int msize;

	if(size%2 == 0)
		msize = size/2;
	else
		msize = size/2 + 1;

	// Create two vectors each containing ip1 and ip2 half of the original input vector 

	std::pair<std::string, std::string> p1;

	std::string nc_err, n_exe_err;

	for(int i = 0; i < msize; i++)
	{
		ip1.push_back(mvar.at(i));
	}

	ip2.push_back(mvar.at(0)); // Since the comparison often happens with the first meta variant and the vector is sorted

	for(int i = msize; i < size; i++)
	{
		ip2.push_back(mvar.at(i));
	}

	// Perform reduction on the first half

	p1 = createTestCase(ip1, output_file);	//New Test case Generated with mvar meta variants
	nc_err = p1.first;
	n_exe_err = p1.second;

	if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
	{
		if(ip1.size() > 2)
		{
			new_mvar1 = verticalReduction(compile_err, exe_err, ip1, output_file);
		}
		else
		{
			new_mvar1 = ip1;
		}

		new_mvar = new_mvar1;
	}
	else //Back Track
	{
		new_mvar1 = ip1;

		p1 = createTestCase(ip2, output_file);	//New Test case Generated with mvar meta variants
		nc_err = p1.first;
		n_exe_err = p1.second;

		if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
		{
			if(ip2.size() > 2)
			{
				new_mvar2 = verticalReduction(compile_err, exe_err, ip2, output_file);
			}
			else
			{
				new_mvar2 = ip2;
			}

			new_mvar = new_mvar2;
		}
		else
		{
			new_mvar2 = ip2;

			new_mvar = verticalReductionMerge(new_mvar1, new_mvar2);

			if(new_mvar != mvar)
			{
				logDebug(fmt::format("Merged output is not same as original"));
				printVectorApiObjects(new_mvar);

				new_mvar = verticalReduction(compile_err, exe_err, new_mvar, output_file);
			}
			else // Restore the old testcase back
			{
				createTestCase(mvar, output_file);
			}
		}
	}

	logDebug(fmt::format("Inside verticalReduction-- Final Output"));
	printVectorApiObjects(new_mvar);

	return new_mvar;
}	

std::vector<const ApiInstructionInterface*> Reduction::MetaVariantReduce(std::vector<const ApiObject*>  var)
{
	int id;
	std::vector<const ApiInstructionInterface*> result, list_inst;

	for(std::vector<const ApiObject*>::iterator it = var.begin(); it != var.end(); it++)
	{
		id = (*it)->getID();

		list_inst = api_fuzzer->MetaVariant_Instr[id];

		for(std::vector<const ApiInstructionInterface*>::iterator iit = list_inst.begin(); iit != list_inst.end(); iit++)
		{
			result.push_back(*iit);
		}
	}		

	return result;
}

void Reduction::fuzzerReduction(std::string compile_err, std::string exe_err, std::string output_file)
{
	std::vector<NodeT*> root = api_fuzzer->tree.getRoots();

	std::vector<const ApiInstructionInterface*> input, input_temp;

//	std::cout << "Number of Roots: " << root.size() << std::endl;
//	printVectorNodes(root);
//	std::cout << "End of Root Nodes" << std::endl;

//	std::cout << "Tree Constructed" << std::endl;
//	printVectorApiInstructions(api_fuzzer->tree.traverse());
//	std::cout << "End of Tree" << std::endl;

//	std::cout << "Number of Edges in the Tree: " << api_fuzzer->tree.edges.size() << std::endl;
//	printVectorEdges(api_fuzzer->tree.edges);
//	std::cout << "End of Edges" << std::endl;


	for(std::vector<NodeT*>::iterator it = root.begin(); it != root.end(); it++)
	{
//		std::cout << "Root: " << (*it)->var->toStr() << std::endl;

		nodeReduction(compile_err, exe_err, *it, output_file);
	}	

	api_fuzzer->FUZ_IP = api_fuzzer->tree.traverse();
}

void Reduction::nodeReduction(std::string compile_err, std::string exe_err, NodeT* node, std::string output_file)
{
	std::vector<EdgeT*> child;
	EdgeT* edge;

	for(std::vector<EdgeT*>::iterator it = api_fuzzer->tree.edges.begin(); it != api_fuzzer->tree.edges.end(); it++)
	{
		edge = *it;

		if(edge->src == node)
		{
			child.push_back(edge);
		}
	}

//	std::cout << "Node Reduction: " << node->var->toStr() << std::endl;

	std::vector<ApiInstructionInterface*> inst;

	std::vector<EdgeT*> new_child = childReduction(compile_err, exe_err, node, child, output_file);

	std::vector<EdgeT*> rem_child;

	std::set_difference(child.begin(), child.end(), new_child.begin(), new_child.end(), std::inserter(rem_child, rem_child.begin()));

	api_fuzzer->tree.removeChildren(rem_child);

//	return api_fuzzer->tree.traverseSubTree(node);
//	printVectorApiInstructions(api_fuzzer->tree.traverse());
}


std::pair<std::string, std::string> Reduction::createTestCaseEdge(NodeT* node, std::vector<EdgeT*> new_child, std::string output_file)
{
//	std::cout << "CreateTestCase for Edges: " << new_child.size() << std::endl;

	DependenceTree new_tree = api_fuzzer->tree;
	
	EdgeT* edge;

	std::vector<EdgeT*> child;

	for(std::vector<EdgeT*>::iterator it = new_tree.edges.begin(); it != new_tree.edges.end(); it++)
	{
		edge = *it;

		if(edge->src == node)
		{
			child.push_back(edge);
		}
	}

	std::vector<EdgeT*> rem_child;

	std::set_difference(child.begin(), child.end(), new_child.begin(), new_child.end(), std::inserter(rem_child, rem_child.begin()));

	new_tree.removeChildren(rem_child);

	std::vector<const ApiInstructionInterface*> inst = new_tree.traverse();

	std::stringstream new_ss_m;

	for(std::vector<const ApiInstructionInterface*>::iterator it = inst.begin(); it != inst.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

	for(std::vector<const ApiInstructionInterface*>::iterator it = api_fuzzer->RED.begin(); it != api_fuzzer->RED.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

	std::ofstream ofs;
	ofs.open(output_file);

	ofs << new_ss_i.str();
	ofs << new_ss_m.str();
	ofs << new_ss_p.str();
	ofs.close();

	std::string new_compile_err = "";	
	std::string new_exe_err = "";	
	std::pair<std::string,std::string> res;	

 	new_compile_err = Exec(command);
	new_exe_err = exeExec(exe_command);

	res = std::make_pair(new_compile_err, new_exe_err);

	return res;
}

std::vector<EdgeT*> Reduction::edgeMerge(std::vector<EdgeT*> mvar1, std::vector<EdgeT*> mvar2)
{
	for(std::vector<EdgeT*>::iterator it = mvar2.begin(); it != mvar2.end(); it++)
	{
		if(find(mvar1.begin(), mvar1.end(), *it) == mvar1.end())
		{
			mvar1.push_back(*it);
		}
	}

	return mvar1;
}

std::vector<EdgeT*> Reduction::childReduction(std::string compile_err, std::string exe_err, NodeT* node, std::vector<EdgeT*> child, std::string output_file)
{
	logDebug(fmt::format("Inside childReduction"));
//	std::cout << "Inside childReduction: " << node->var->toStr() << std::endl;
//	printVectorEdges(child);
//	std::cout << "End" << std::endl;

	if(child.size() <= 1)
	{
		createTestCaseEdge(node, child, output_file);
		return child;
	}

	std::vector<EdgeT*> new_child1, new_child2, ip1, ip2, new_child;

	//Find the mid of the input vector child

	int size = child.size();
	int msize;

	if(size%2 == 0)
		msize = size/2;
	else
		msize = size/2 + 1;

	// Create two vectors each containing ip1 and ip2 half of the original input vector 

	std::pair<std::string, std::string> p1;

	std::string nc_err, n_exe_err;

	for(int i = 0; i < msize; i++)
	{
		ip1.push_back(child.at(i));
	}

	ip2.push_back(child.at(0));

	for(int i = msize; i < size; i++)
	{
		ip2.push_back(child.at(i));
	}

	// Perform reduction on the first half

	p1 = createTestCaseEdge(node, ip1, output_file);	//New Test case Generated with mvar meta variants

//	std::cout << "Created test case with the first half" <<std::endl;

	nc_err = p1.first;
	n_exe_err = p1.second;

	if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
	{
//		std::cout << "Reduce test case with the first half" <<std::endl;

		if(ip1.size() > 1)
		{
			new_child1 = childReduction(compile_err, exe_err, node, ip1, output_file);
		}
		else
		{
			new_child1 = ip1;
		}

		new_child = new_child1;
	}
	else //Back Track
	{
//		std::cout << "Test case failed with the first half" <<std::endl;

		new_child1 = ip1;
	
		if(ip2 == child)
		{
			createTestCaseEdge(node, ip2, output_file);
			return child;
		}

		p1 = createTestCaseEdge(node, ip2, output_file);	//New Test case Generated with mvar meta variants
	
//		std::cout << "Created test case with the second half" <<std::endl;

		nc_err = p1.first;
		n_exe_err = p1.second;

		if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
		{
//			std::cout << "Reduce test case with the second half" <<std::endl;

			if(ip2.size() > 1)
			{
				new_child2 = childReduction(compile_err, exe_err, node, ip2, output_file);
			}
			else
			{
				new_child2 = ip2;
			}

			new_child = new_child2;
		}
		else
		{
			new_child2 = ip2;

//			std::cout << "Test case failed with the second half" <<std::endl;

			new_child = edgeMerge(new_child1, new_child2);

//			std::cout << "Merged Output" << std::endl;
			logDebug(fmt::format("Merged Output"));
//			printVectorEdges(new_child);

			if(new_child != child)
			{
				new_child = childReduction(compile_err, exe_err, node, new_child, output_file);
			}
			else
			{
				createTestCaseEdge(node, child, output_file);
			}
		}
	}

//	std::cout << "Final Output" << std::endl;
	logDebug(fmt::format("Final Output"));
	printVectorEdges(new_child);
//	std::cout << "End Final" << std::endl;

	return new_child;
}

const ApiObject* Reduction::getReplacementObject(const ApiType* type)
{
	#if 0
	const ApiObject* obj;
	int x;

	x = this->depth;
	this->depth = this->max_depth+1;

	obj = generateObject(type);
	const ApiInstructionInterface* instrNode;
	instrNode = this->instrs.at(this->instrs.size()-1);	
	
//	std::cout << "New Object: " << obj->toStr() << std::endl;
//	std::cout << "instruction inserted: " << instrNode->toStr() << std::endl;

	this->depth = x;

	return obj;
	#endif

	#if 1	
	std::vector<const ApiObject*> leaf_objs, f_leaf_objs;	

	leaf_objs = api_fuzzer->tree.getLeafNodes();

	f_leaf_objs = filterObjList(leaf_objs, &ApiObject::hasType, type);

	const ApiObject* res = NULL;

	if(!f_leaf_objs.empty())
		res = f_leaf_objs.at(0);

	#if 0
	std::vector<NodeT*> roots;
	roots = api_fuzzer->tree.getRoots();

	int min = 0;
	std::vector<const ApiInstructionInterface*> instrs;

	for(std::vector<NodeT*>::iterator rit = roots.begin(); rit != roots.end(); rit++)
	{
		for(std::vector<const ApiObject*>::iterator it = f_leaf_objs.begin(); it != f_leaf_objs.end(); it++)
		{
			instrs = api_fuzzer->tree.traverseBetweenTwoNodes((*rit)->var, *it);

			if(min < instrs.size())
			{
				min = instrs.size();
				res = *it;	
			}
		}
	}
	#endif

	return res;
	#endif
}

void Reduction::reduceSubTree(std::string compile_err, std::string exe_err, std::string output_file)
{

	std::vector<NodeT*> root = api_fuzzer->tree.getRoots();

	std::vector<NodeT*> nodes, temp, vec_nodes;
	std::vector<EdgeT*> vec_edges, vec_edges_temp;
	const ApiObject* obj = NULL;
	const ApiType* type;	

	#if 0
	std::cout << "Inside reduceSubTree" << std::endl;
	api_fuzzer->tree.traverse();
	std::cout << "Traverse Ends" << std::endl;
	#endif

	for(std::vector<NodeT*>::iterator it = root.begin(); it != root.end(); it++)
	{
//		std::cout << "Root: " << (*it)->var->toStr() << std::endl;

		vec_nodes = api_fuzzer->tree.getDescendants(*it);

		int max = 0;
		NodeT* node = NULL;

		for(std::vector<NodeT*>::iterator nit = vec_nodes.begin(); nit != vec_nodes.end(); nit++)
		{
			if(*nit != *it)
			{
				if((*nit)->var->getType()->isPrimitive())
				{
					continue;
				}

				subTreeReduction(compile_err, exe_err, *nit, output_file);
			}
		}
	}

	api_fuzzer->FUZ_IP = api_fuzzer->tree.traverse();
}

void Reduction::subTreeReduction(std::string compile_err, std::string exe_err, NodeT* node, std::string output_file)
{
	std::vector<NodeT*> temp;
	std::vector<EdgeT*> vec_edges, vec_edges_temp;
	NodeT* new_node;

        const ApiObject* obj;

//	std::cout << "Inside subTreeReduction for Node: " << node->var->toStr() << std::endl;

	obj = getReplacementObject(node->var->getType());

	if(obj == NULL)
	{
//		std::cout << "Replacement Object is NULL " << std::endl;
		return;
	}

//	std::cout << "Replacement Object: " << obj->toStr() << std::endl;

	DependenceTree new_tree = api_fuzzer->tree;

	NodeT* new_node1 = new_tree.insertNode(obj);

//	std::cout << "Inside subTreeReduction" << std::endl;

	new_tree = replaceSubTree(new_tree, node, new_node1); 

//	std::cout << "Inside subTreeReduction -- after replaceSubTree" << std::endl;

//	printVectorEdges(new_edges);

//	new_tree.traverse();
//	std::cout << "Traverse End" << std::endl;

	std::pair<std::string, std::string> res;

	res = createTestCaseTree(new_tree, output_file);

	std::string nc_err = res.first;
	std::string n_exe_err = res.second;

	NodeT* new_node2;

	const ApiObject* n_obj = NULL;
	const ApiType* n_type;

	if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
	{
//		std::cout << "Test Case Reduced retains the error" << std::endl;

		api_fuzzer->tree = new_tree;
	}
	else
	{
//		std::cout << "Choose some other node at the next level" << std::endl;

		vec_edges = api_fuzzer->tree.getImmDescendants(node);

		int max = 0;
		NodeT* enode = NULL;
		std::vector<NodeT*> nodes;

		for(std::vector<EdgeT*>::iterator it = vec_edges.begin(); it != vec_edges.end(); it++)
		{
			temp = (*it)->dests;	

			for(std::vector<NodeT*>::iterator nit = temp.begin(); nit != temp.end(); nit++)
			{
				if(find(nodes.begin(), nodes.end(), *nit) == nodes.end())
				{
					if(*nit != node)
					{
						if((*nit)->var->getType()->isPrimitive())
						{
							continue;
						}

						vec_edges_temp = api_fuzzer->tree.getImmDescendants(*nit);

						if(vec_edges_temp.size() > max)
						{
							max = vec_edges.size();
							enode = *nit;
						}

						nodes.push_back(*nit);
					}
				}
			}

		}

		if(enode != NULL)
		{
			subTreeReduction(compile_err, exe_err, enode, output_file);
		}
		else if(!nodes.empty())
		{
			enode = nodes.at(rand()%nodes.size());

			subTreeReduction(compile_err, exe_err, enode, output_file);
		}

		// CHECK
		createTestCaseTree(api_fuzzer->tree, output_file); // Reverting back to original test case
	}
}

std::pair<std::string, std::string> Reduction::createTestCaseTree(DependenceTree tree, std::string output_file)
{
	std::vector<const ApiInstructionInterface*> inst;

	std::stringstream new_ss_m;

	inst = tree.traverse();

	for(std::vector<const ApiInstructionInterface*>::iterator it = inst.begin(); it != inst.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

	for(std::vector<const ApiInstructionInterface*>::iterator it = api_fuzzer->RED.begin(); it != api_fuzzer->RED.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

	std::ofstream ofs;
	ofs.open(output_file);

	ofs << new_ss_i.str();
	ofs << new_ss_m.str();
	ofs << new_ss_p.str();
	ofs.close();

	std::string new_compile_err = "";	
	std::string new_exe_err = "";	
	std::pair<std::string,std::string> res;	

 	new_compile_err = Exec(command);
	new_exe_err = exeExec(exe_command);

	res = std::make_pair(new_compile_err, new_exe_err);

	return res;

}

void Reduction::simplifyMetaRelationsPrep(std::string compile_err, std::string exe_err, std::string output_file)
{
//	std::cout << "Inside simplifyMetaRelations: " << api_fuzzer->mvar_relations.size() << std::endl;

	const ApiInstructionInterface* new_instr;
	const ApiInstructionInterface* instr1;
	const ApiInstructionInterface* instr2;

	std::vector<const MetaRelation*> rel1, rel2;
	const ApiObject* mvar;

	std::map<const ApiInstructionInterface*, const ApiInstructionInterface*> map_relations;
	
	for(std::map<const ApiObject*, original_simplified_mapping>::reverse_iterator it = api_fuzzer->mvar_relations.rbegin(); it != api_fuzzer->mvar_relations.rend(); it++)
	{
		mvar = it->first;

		if(find(api_fuzzer->MVAR.begin(), api_fuzzer->MVAR.end(), mvar) == api_fuzzer->MVAR.end())
		{
			continue;
		}

//		std::cout << "Mvar: " << mvar->toStr() << std::endl;
	
		rel1 = it->second.first;
		rel2 = it->second.second;

//		std::cout << "Rel1 Size: " << rel1.size() << std::endl;
//		std::cout << "Rel2 Size: " << rel2.size() << std::endl;

		for(int i = rel1.size()-1; i >= 0; i--)
		{
//			std::cout << "i: " << i << std::endl;

//			std::cout << "Rel1: " << rel1.at(i)->toStr() << std::endl;
	
			if(rel1.at(i)->getAbstractRelation() == "check")
			{
				instr1 = rel1.at(i)->toApiInstruction();

				map_relations[instr1] = instr1;

				continue;
			}	

			mvar->setDeclared();
			instr1 = rel1.at(i)->toApiInstruction(); // Without Declaration
			mvar->resetDeclared();
			instr2 = rel1.at(i)->toApiInstruction();
		
			if(isPresent(api_fuzzer->RED, instr1) != NULL)
			{
				mvar->setDeclared();

				new_instr = rel2.at(i)->toApiInstruction();
				map_relations[instr1] = new_instr;
			}	
			else if(isPresent(api_fuzzer->RED, instr2) != NULL)
			{
				mvar->resetDeclared();

				new_instr = rel2.at(i)->toApiInstruction();
				map_relations[instr2] = new_instr;
			}	
		}

//		std::cout << "Outside Inner for loop" << std::endl;
	}

//	std::cout << "Outside Outer for loop" << std::endl;

	std::vector<const ApiInstructionInterface*> res;

//	printVectorApiInstructions(red);

	try_outs = 0;

	#if 0
	std::vector<const ApiInstructionInterface*> simp_red;

	for(int i = 0; i < red.size(); i++)
	{
		if(i%2 == 0)
		{
			simp_red.push_back(red.at(i));
		}
	}
	#endif

	res = simplifyMetaRelations(compile_err, exe_err, output_file, api_fuzzer->RED, map_relations);

	api_fuzzer->RED = res;
	
	#if 0
 	std::string new_compile_err = Exec(command);
	std::string new_exe_err = exeExec(exe_command);

	if(checkTestCase(compile_err, new_compile_err, exe_err, new_exe_err))
	{
		return res;
	}
	else
	{
		createTestCaseSimplify(input_insts, red, red, map_relations, output_file);
		return res;
	}
	#endif
}

std::pair<std::string, std::string> Reduction::createTestCaseSimplify(std::vector<const ApiInstructionInterface*> red, std::map<const ApiInstructionInterface*, const ApiInstructionInterface*> map_relations, std::string output_file)
{
	std::vector<const ApiInstructionInterface*> inst;

	std::stringstream new_ss_m;

	for(std::vector<const ApiInstructionInterface*>::iterator it = api_fuzzer->FUZ_IP.begin(); it != api_fuzzer->FUZ_IP.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

//	std::cout << "Create test case" << std::endl;
//	printVectorApiInstructions(var);

	for(std::vector<const ApiInstructionInterface*>::iterator it = red.begin(); it != red.end(); it++)
	{
//		inst.push_back(map_relations[*it]);

//		std::cout << "Instr in red: " << (*it)->toStr() << std::endl;

		if(isPresent(api_fuzzer->RED, *it) != NULL)	
		{
			if(map_relations.find(*it) != map_relations.end())	
			{
//				std::cout << "Instr Found: " << std::endl;
//				std::cout << "New Instr: " << map_relations[*it]->toStr() << std::endl;
				writeLine(new_ss_m, map_relations[*it]->toStr());
			}
			else
			{
				writeLine(new_ss_m, (*it)->toStr());
			}
		}
		else
		{
//			std::cout << "Instr Not Found: " << std::endl;
			writeLine(new_ss_m, (*it)->toStr());
		}
	}	

	std::ofstream ofs;
	ofs.open(output_file);

	ofs << new_ss_i.str();
	ofs << new_ss_m.str();
	ofs << new_ss_p.str();
	ofs.close();

	std::string new_compile_err = "";	
	std::string new_exe_err = "";	
	std::pair<std::string,std::string> res;	

 	new_compile_err = Exec(command);
	new_exe_err = exeExec(exe_command);

	res = std::make_pair(new_compile_err, new_exe_err);

	return res;
}

std::vector<const ApiInstructionInterface*> Reduction::simplifyMetaRelations(std::string compile_err, std::string exe_err, std::string output_file, std::vector<const ApiInstructionInterface*> red, std::map<const ApiInstructionInterface*, const ApiInstructionInterface*> map_relations)
{
	if(try_outs > 10)
	{
		map_relations.clear();
		createTestCaseSimplify(red, map_relations, output_file);
		return red;
	}

	try_outs++;

	std::pair<std::string, std::string> p1;

	std::string nc_err, n_exe_err;

	p1 = createTestCaseSimplify(red, map_relations, output_file);

	nc_err = p1.first;
	n_exe_err = p1.second;

	if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
	{
		return red;
	}

	if(red.size() <= 1)
	{
		return red;
	}
	
	std::vector<const ApiInstructionInterface*> new_red, ip1, ip2, new_red1, new_red2;

	int size = red.size();
	int msize;

	if(size%2 == 0)
		msize = size/2;
	else
		msize = size/2 + 1;

	// Create two vectors each containing ip1 and ip2 half of the original input vector 

	for(int i = 0; i < msize; i++)
	{
		ip1.push_back(red.at(i));
	}

//	ip2.push_back(child.at(0));

	for(int i = msize; i < size; i++)
	{
		ip2.push_back(red.at(i));
	}

	// Perform reduction on the first half

	p1 = createTestCaseSimplify(ip1, map_relations, output_file);

//	std::cout << "Created test case with the first half" <<std::endl;

	nc_err = p1.first;
	n_exe_err = p1.second;

	if(!checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
	{
//		std::cout << "Reduce test case with the first half" <<std::endl;

		if(ip1.size() > 1)
		{
			new_red1 = simplifyMetaRelations(compile_err, exe_err, output_file, ip1, map_relations);
		}
		else
		{
			new_red1 = ip1;
		}

		new_red = new_red1;
	}
	else //Back Track
	{
//		std::cout << "Test case failed with the first half" <<std::endl;

		new_red1 = ip1;
	
		p1 = createTestCaseSimplify(ip2, map_relations, output_file);	//New Test case Generated with mvar meta variants
	
//		std::cout << "Created test case with the second half" <<std::endl;

		nc_err = p1.first;
		n_exe_err = p1.second;

		if(!checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
		{
//			std::cout << "Reduce test case with the second half" <<std::endl;

			if(ip2.size() > 1)
			{
				new_red2 = simplifyMetaRelations(compile_err, exe_err, output_file, ip2, map_relations);
			}
			else
			{
				new_red2 = ip2;
			}

			new_red = new_red2;
		}
		else
		{
			new_red2 = ip2;

//			std::cout << "Test case failed with the second half" <<std::endl;

			new_red = instructionMerge(new_red1, new_red2);

			if(new_red != red)
			{
				new_red = simplifyMetaRelations(compile_err, exe_err, output_file, new_red, map_relations);
			}
			else
			{
				createTestCaseSimplify(red, map_relations, output_file);
			}
		}
	}

	return new_red;
}

const ApiInstructionInterface* Reduction::getNewInstruction(const ApiInstruction* old_instr, NodeT* node, NodeT* new_node)
{
	const ApiObject* target;
	const ApiObject* obj;
	const ApiInstruction* new_instr;
	std::vector<const ApiObject*> params, new_params;

//	std::cout << "Hey Der" << std::endl;

	target = old_instr->getTargetObj();

	params = old_instr->getFuncParams();

//	printVectorApiObjects(params);

	if(target != NULL && target == node->var)
	{
//		std::cout << "Node is Target" << target->toStr() << std::endl;		
		target  = new_node->var;
		new_params = params;
	}
	else
	{
//		std::cout << "Node is Param: " << params.size() << std::endl;		
		for(std::vector<const ApiObject*>::iterator it = params.begin(); it != params.end(); it++)
		{
//			std::cout << "Par: " << (*it)->toStr() << std::endl;

			if(*it == node->var)
			{
//				std::cout << "Match" << std::endl;
//				std::cout << "New Node: "  << new_node->var->toStr() << std::endl;
				new_params.push_back(new_node->var);
			}
			else
			{
				new_params.push_back(*it);
			}
		}
	}

//	printVectorApiObjects(new_params);

	obj = old_instr->getResultObj();

//	std::cout << "Result: " << obj->toStr() << std::endl;

	const ApiInstruction* e_instr;
	e_instr = dynamic_cast<const ApiInstruction*>(old_instr);

        if(e_instr->isDeclInstr())
	{
		obj->resetDeclared();
	}

	new_instr = new ApiInstruction(old_instr->getFunc(), obj, target, new_params);

	return ((const ApiInstructionInterface*)(new_instr));
}

EdgeT* Reduction::getNewEdge(EdgeT* old_edge, NodeT* node, NodeT* new_node)
{
	const ApiInstruction* old_instr;
	const ApiInstructionInterface* new_instr;

	#if 0
	std::cout << "Inside New Edge" << std::endl;
	std::cout << "Node: " << node->var->toStr() << std::endl;
	std::cout << "New Node: " << new_node->var->toStr() << std::endl;
	#endif

	std::vector<NodeT*> new_dests, old_dests;
	NodeT* target_node;

	EdgeT* res = new EdgeT();

	res->src = old_edge->src;

	old_dests = old_edge->dests;

	for(std::vector<NodeT*>::iterator it = old_dests.begin(); it != old_dests.end(); it++)
	{
		if(*it == node)
		{
//			std::cout << "Match Found" << std::endl;
			new_dests.push_back(new_node);			
		}
		else
		{
			new_dests.push_back(*it);
		}
	}

	printVectorNodes(new_dests);

	res->dests = new_dests;

	old_instr = dynamic_cast<const ApiInstruction*>(old_edge->instr);

	new_instr = getNewInstruction(old_instr, node, new_node);
	
	res->instr = (const ApiInstructionInterface*)(new_instr);

	return res;
}

DependenceTree Reduction::replaceSubTree(DependenceTree tree, NodeT* node, NodeT* new_node)
{
//	std::cout << "Inside replaceSubTree" << std::endl;
//	std::cout <<  "New Node: " << new_node->var->toStr() << std::endl; 

	DependenceTree new_tree = tree;

	std::vector<EdgeT*> ancestors, descendants;
	std::vector<EdgeT*> res;

	ancestors = new_tree.getImmAncestors(node);
	descendants = new_tree.getImmDescendants(node);

	EdgeT* old_edge;
	EdgeT* new_edge;

	std::vector<EdgeT*> edges = new_tree.edges;

	for(std::vector<EdgeT*>::iterator it = edges.begin(); it != edges.end(); it++)
	{
		if(find(ancestors.begin(), ancestors.end(), *it) != ancestors.end())
		{
			old_edge = *it;

//			std::cout << "Ancestor Edge" << std::endl;

			new_edge = getNewEdge(old_edge, node, new_node);

			#if 0
			std::cout << "Old Edge" << std::endl;
			printEdge(old_edge);
			std::cout << "New Edge" << std::endl;
			printEdge(new_edge);
			#endif

			res.push_back(new_edge);
		}	
		else if(find(descendants.begin(), descendants.end(), *it) != descendants.end())
		{
//			std::cout << "Descendant Edge" << std::endl;
//			printEdge(*it);
		}
		else
		{
//			std::cout << "No Edge" << std::endl;

			res.push_back(*it);
		}
	}

//	std::cout << "Modified Edges" << std::endl;
//	printVectorEdges(res);

	new_tree.edges = res;

	return new_tree;
}

void Reduction::replaceMetaInputVariables(std::string compile_err, std::string exe_err, std::string output_file)
{
	std::vector<NodeT*> roots = api_fuzzer->tree.getRoots();
	const ApiObject* obj = NULL;
	const ApiType* type;
	const ApiObject* result_obj;
	DependenceTree new_tree;
	NodeT* root = NULL;
	std::pair<std::string, std::string> res;
	std::string new_compile_err = "";
	std::string new_exe_err = "";	
	const ApiInstructionInterface* old_instr;
	const ApiInstructionInterface* new_instr;
	std::vector<const ApiInstructionInterface*> new_red, temp_red;

	for(std::vector<NodeT*>::iterator it = roots.begin(); it != roots.end(); it++)
	{
		root = *it;

		temp_red = api_fuzzer->tree.traverseSubTree(root);

//		std::cout << "Replacing Meta Input Variable: " << root->var->toStr() << std::endl;

		type = root->var->getType();

		obj = getReplacementObject(type);

		if(obj == NULL)
		{
			continue;
		}

//		std::cout << "Replacing Meta Input Variable by Object: " << obj->toStr() << std::endl;

		for(std::vector<const ApiInstructionInterface*>::iterator rit = api_fuzzer->RED.begin(); rit != api_fuzzer->RED.end(); rit++)
		{
			old_instr = *rit;

//			std::cout << "Old Instr: " << old_instr->toStr() << std::endl;

			new_instr = getNewInstructionForMetaRelation(old_instr, root->var, obj);

//			std::cout << "New Instr: " << new_instr->toStr() << std::endl;
			
			new_red.push_back(new_instr);	
		}

		#if 0
		std::cout << "Red Before: " << red.size() << std::endl;
		printVectorApiInstructions(red);
		std::cout << "Red After: " << new_red.size() << std::endl;
		printVectorApiInstructions(new_red);
		#endif

		new_tree = api_fuzzer->tree;

//		std::cout << "Hey der" << std::endl;
		new_tree.removeRootNode(root);

//		std::cout << "Tree after deleting root: " << root->var->toStr() << std::endl;
//		printVectorApiInstructions(new_tree.traverse());

		res = createTestCaseForInputVarReduction(new_red, output_file, new_tree);

		new_compile_err = res.first;
		new_exe_err = res.second;

		if(checkTestCase(compile_err, new_compile_err, exe_err, new_exe_err))
		{
//			std::cout << "Test Case retained error after deleting root: " << root->var->toStr() << std::endl;
			api_fuzzer->tree = new_tree;
			api_fuzzer->RED = new_red;
		}
		#if 0
		else
		{
			std::cout << "Test Case error after deleting root is gone: " << root->var->toStr() << std::endl;
		}
		#endif	
	}

	api_fuzzer->FUZ_IP = api_fuzzer->tree.traverse();
}

std::pair<std::string, std::string> Reduction::createTestCaseForInputVarReduction(std::vector<const ApiInstructionInterface*> red, std::string output_file, DependenceTree tree)
{
	std::vector<const ApiInstructionInterface*> list_inst;
	std::stringstream new_ss_m;

	list_inst = tree.traverse();

	for(std::vector<const ApiInstructionInterface*>::iterator it = list_inst.begin(); it != list_inst.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

	for(std::vector<const ApiInstructionInterface*>::iterator it = red.begin(); it != red.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

	std::ofstream ofs;
	ofs.open(output_file);

	ofs << new_ss_i.str();
	ofs << new_ss_m.str();
	ofs << new_ss_p.str();
	ofs.close();

	std::string new_compile_err = "";	
	std::string new_exe_err = "";	
	std::pair<std::string,std::string> res;	

 	new_compile_err = Exec(command);
	new_exe_err = exeExec(exe_command);

	res = std::make_pair(new_compile_err, new_exe_err);

	return res;
}

const FuncObject* Reduction::processFuncObject(const FuncObject* f_obj, const ApiObject* original_obj, const ApiObject* new_obj)
{
	const ApiObject* target = NULL;
	const FuncObject* func_obj;
	std::vector<const ApiObject*> all_objs;
	bool api_obj;

//	std::cout << "inside processFuncObject: " << f_obj->toStr() << std::endl;

	target = f_obj->getTarget();

	if(target != NULL)
	{
//		std::cout << "Target: " << target->toStr() << std::endl;
	
		api_obj = false;

		all_objs = target->getAllObjs();

		if(all_objs.size() == 1)
		{
			if(all_objs.at(0) == target)
			{
				api_obj = true;
			}
		}

		if(!api_obj) // Func Object
		{
			func_obj = (const FuncObject*)(target);

			func_obj = processFuncObject(func_obj, original_obj, new_obj);
			
			target = dynamic_cast<const ApiObject*>(func_obj);
		}
		else
		{
			if(target == original_obj)
			{
				target = new_obj;
			}
		}

//		std::cout << "New Target: " << target->toStr() << std::endl;
	}

	std::vector<const ApiObject*> params, new_params;
	const ApiObject* parameter = NULL;

	params = f_obj->getParams();

	for(std::vector<const ApiObject*>::iterator it = params.begin(); it != params.end(); it++)
	{
		parameter = *it;
		api_obj = false;	

		if(parameter != NULL)
		{
//			std::cout << "Parameter: " << parameter->toStr() << std::endl;

			all_objs = parameter->getAllObjs();

//			std::cout << "all_objs size: " << all_objs.size() << " all_objs.at(0): " << all_objs.at(0)->toStr() << std::endl;

			if(all_objs.size() == 1)
			{
				if(all_objs.at(0) == parameter)
				{
					api_obj = true;
				}
			}

			if(!api_obj) // Func Object
			{
				func_obj = (const FuncObject*)(parameter);

//				std::cout << "Func Object" << std::endl;

				func_obj = processFuncObject(func_obj, original_obj, new_obj);
			
				parameter = dynamic_cast<const ApiObject*>(func_obj);
			}
			else
			{
//				std::cout << "!Func Object" << std::endl;
				if(parameter == original_obj)
				{
					parameter = new_obj;
				}
			}
		}

//		std::cout << "New Parameter: " << parameter->toStr() << std::endl;

		new_params.push_back(parameter);
	}

	const FuncObject* new_f_obj = NULL;

	new_f_obj = new FuncObject(f_obj->getFunc(), target, new_params);

	return new_f_obj;
}


const ApiInstructionInterface* Reduction::getNewInstructionForMetaRelation(const ApiInstructionInterface* instr, const ApiObject* original_obj, const ApiObject* new_obj)
{
	const ApiInstruction* old_instr = NULL;

	old_instr = dynamic_cast<const ApiInstruction*>(instr);

	if(old_instr == NULL)
	{
		return instr;
	}
	else if(old_instr->getFunc()->getName() == "assert")
	{
		return instr;
	}

//	std::cout << "Old Instruction: " << old_instr->toStr() << std::endl;

	const ApiObject* target = NULL;
	std::vector<const ApiObject*> all_objs;
	const FuncObject* func_obj;
	bool api_obj;

	target = old_instr->getTargetObj();

	if(target != NULL)
	{
		api_obj = false;

		all_objs = target->getAllObjs();

		if(all_objs.size() == 1)
		{
			if(all_objs.at(0) == target)
			{
				api_obj = true;
			}
		}

		if(!api_obj) // Func Object
		{
			func_obj = (const FuncObject*)(target);

			func_obj = processFuncObject(func_obj, original_obj, new_obj);
			
			target = dynamic_cast<const ApiObject*>(func_obj);
		}
		else
		{
			if(target == original_obj)
			{
				target = new_obj;
			}
		}
	}

	std::vector<const ApiObject*> params, new_params;
	const ApiObject* parameter = NULL;

	params = old_instr->getFuncParams();

	for(std::vector<const ApiObject*>::iterator it = params.begin(); it != params.end(); it++)
	{
		parameter = *it;

		api_obj = false;

		if(parameter != NULL)
		{
			all_objs = parameter->getAllObjs();

			if(all_objs.size() == 1)
			{
				if(all_objs.at(0) == parameter)
				{
					api_obj = true;
				}
			}

			if(!api_obj) // Func Object
			{
				func_obj = (const FuncObject*)(parameter);

//				std::cout << "Parameter FuncObject: " << func_obj->toStr() << std::endl;

				func_obj = processFuncObject(func_obj, original_obj, new_obj);
	
//				std::cout << "New Parameter Func Object: " << func_obj->toStr() << std::endl;
			
				parameter = dynamic_cast<const ApiObject*>(func_obj);
			}
			else
			{
				if(parameter == original_obj)
				{
					parameter = new_obj;
				}
			}
		}

		new_params.push_back(parameter);
	}

	const ApiObject* obj = NULL;
	const ApiInstruction* new_instr;

	obj = old_instr->getResultObj();

	if(obj)
	{
//		std::cout << "Result: " << obj->toStr() << std::endl;
	
		const ApiInstruction* e_instr;
		e_instr = dynamic_cast<const ApiInstruction*>(old_instr);

        	if(e_instr->isDeclInstr())
		{
			obj->resetDeclared();
		}
	}

	new_instr = new ApiInstruction(old_instr->getFunc(), obj, target, new_params);

//	std::cout << "New Instruction: " << new_instr->toStr() << std::endl;

	return ((const ApiInstructionInterface*)(new_instr));
}

std::pair<std::string, std::string> Reduction::createTestCase(std::vector<const ApiObject*> var, std::string output_file)
{
	std::vector<const ApiInstructionInterface*> list_inst;
	std::stringstream new_ss_m;

	for(std::vector<const ApiInstructionInterface*>::iterator it = api_fuzzer->FUZ_IP.begin(); it != api_fuzzer->FUZ_IP.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

	list_inst = MetaVariantReduce(var);

	for(std::vector<const ApiInstructionInterface*>::iterator it = list_inst.begin(); it != list_inst.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

	std::ofstream ofs;
	ofs.open(output_file);

	ofs << new_ss_i.str();
	ofs << new_ss_m.str();
	ofs << new_ss_p.str();
	ofs.close();

	std::string new_compile_err = "";	
	std::string new_exe_err = "";	
	std::pair<std::string,std::string> res;	

 	new_compile_err = Exec(command);
	new_exe_err = exeExec(exe_command);

	res = std::make_pair(new_compile_err, new_exe_err);

	return res;
}

bool Reduction::assertReduction(const ApiInstructionInterface* assert, std::vector<const ApiObject*> var)
{
	std::vector<const ApiObject*> params;
	const ApiInstruction* instr;

	instr = dynamic_cast<const ApiInstruction*>(assert);
	
        if (!strcmp(instr->getFunc()->getName().c_str(), "assert"))
	{
		params = (*(instr->getFuncParams().begin()))->getAllObjs();

		std::vector<const ApiObject*>::iterator it;	

		const ApiObject *first, *second;

		it = params.begin();

		first = *(it);

		it++;

		second = (*it);

		if(first == second)
		{
			return false;
		}

		for(it = params.begin(); it != params.end(); it++)
		{
			if(std::find(var.begin(), var.end(), *it) == var.end())
			{
				return false;
			}
		}

		return true;
	}
	
	return false;
}

std::vector<int> Reduction::MHReduceInstr(std::string compile_err, std::string exe_err, std::vector<int> rel_indices, std::string output_file)
{
	if(rel_indices.size() <= 1)
	{
		createTestCaseMHReduce(exe_err, rel_indices, output_file);
		return rel_indices;
	}

	std::pair<std::string, std::string> p1;

	std::string nc_err, n_exe_err;

	std::vector<int> new_indices, ip1, ip2, new_indices1, new_indices2;

	int size = rel_indices.size();
	int msize;

	if(size%2 == 0)
		msize = size/2;
	else
		msize = size/2 + 1;

	// Create two vectors each containing ip1 and ip2 half of the original input vector 

	for(int i = 0; i < msize; i++)
	{
		ip1.push_back(rel_indices.at(i));
	}

	ip2.push_back(rel_indices.at(0));

	for(int i = msize; i < size; i++)
	{
		ip2.push_back(rel_indices.at(i));
	}

	// Perform reduction on the first half

	p1 = createTestCaseMHReduce(exe_err, ip1, output_file);

	nc_err = p1.first;
	n_exe_err = p1.second;

	if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
	{
		if(ip1.size() > 1)
		{
			new_indices1 = MHReduceInstr(compile_err, exe_err, ip1, output_file);
		}
		else
		{
			new_indices1 = ip1;
		}

		new_indices = new_indices1;
	}
	else //Back Track
	{
		if(ip2 == rel_indices)
		{
			createTestCaseMHReduce(exe_err, ip2, output_file); //New Test case Generated with mvar meta variants
			return ip2;
		}

		new_indices1 = ip1;
	
		p1 = createTestCaseMHReduce(exe_err, ip2, output_file); //New Test case Generated with mvar meta variants
	
		nc_err = p1.first;
		n_exe_err = p1.second;

		if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
		{
			if(ip2.size() > 1)
			{
				new_indices2 = MHReduceInstr(compile_err, exe_err, ip2, output_file);
			}
			else
			{
				new_indices2 = ip2;
			}

			new_indices = new_indices2;
		}
		else
		{
			new_indices2 = ip2;

			new_indices = indexMerge(new_indices1, new_indices2);

			if(new_indices != rel_indices)
			{
				new_indices = MHReduceInstr(compile_err, exe_err, new_indices, output_file);
			}
			else
                        {
                                createTestCaseMHReduce(exe_err, rel_indices, output_file);
                        }
		}
	}

	return new_indices;
}

std::vector<int> Reduction::indexMerge(std::vector<int> v1, std::vector<int> v2)
{
	for(std::vector<int>::iterator it = v2.begin(); it != v2.end(); it++)
	{
		if(find(v1.begin(), v1.end(), *it) == v1.end())
		{
			v1.push_back(*it);
		}
	}

	return v1;
}

std::pair<std::string, std::string> Reduction::createTestCaseMHReduce(std::string exe_err, std::vector<int> rel_indices, std::string output_file)
{
	std::vector<const ApiInstructionInterface*> temp;
	std::stringstream new_ss_m;
	const ApiObject* mvar;
	std::pair<std::string,std::string> res;	

	std::vector<const ApiObject*> assert_vars;
	std::vector<std::string> assert_vars_names;

//    	res = parseErrorMsg(exe_err);

	std::string var1 = "", var2 = "";

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

        assert_vars_names.push_back(var1);
        assert_vars_names.push_back(var2);

//	assert_vars_names.push_back(res.first);
//	assert_vars_names.push_back(res.second);

	assert_vars = getApiObjects(assert_vars_names);

	for(std::vector<const ApiInstructionInterface*>::iterator it = api_fuzzer->FUZ_IP.begin(); it != api_fuzzer->FUZ_IP.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

	for(std::vector<const ApiObject*>::iterator it = api_fuzzer->MVAR.begin(); it != api_fuzzer->MVAR.end(); it++)
	{
		mvar = *it;

		temp =  api_fuzzer->MetaVariant_Instr[mvar->getID()];

		writeLine(new_ss_m, temp.at(0)->toStr());
//		writeLine(new_ss_m, temp.at(1)->toStr());

		for(int i = 0; i < rel_indices.size(); i++)
		{
			writeLine(new_ss_m, temp.at(rel_indices.at(i))->toStr());
		}

		if(var1 != "" && var2 != "")
		{
			if(assertReduction(temp.at(api_fuzzer->meta_test_count+1), assert_vars))
			{
				writeLine(new_ss_m, temp.at(api_fuzzer->meta_test_count+1)->toStr());
			}
		}
		else
		{
			writeLine(new_ss_m, temp.at(api_fuzzer->meta_test_count+1)->toStr());
		}	
	}

	std::ofstream ofs;
	ofs.open(output_file);

	ofs << new_ss_i.str();
	ofs << new_ss_m.str();
	ofs << new_ss_p.str();
	ofs.close();

	std::string new_compile_err = "";	
	std::string new_exe_err = "";	

    	new_compile_err = Exec(command);
    	new_exe_err = exeExec(exe_command);

	res = std::make_pair(new_compile_err, new_exe_err);

	return res;
}

void Reduction::MHReduceInstrPrep(std::string compile_err, std::string exe_err, std::string output_file)
{
	const ApiObject* mvar;
        std::vector<const ApiInstructionInterface*> res, temp;
	std::vector<int> indices;

	for(int i = 1; i <= api_fuzzer->meta_test_count; i++)
	{
		indices.push_back(i);
	}

	if(!isAlertError(exe_err))
	{
		indices.push_back(api_fuzzer->meta_test_count+1);
	}

	indices = MHReduceInstr(compile_err, exe_err, indices, output_file);

	std::pair<std::string,std::string> parsed_msg;	

	std::vector<const ApiObject*> assert_vars;
	std::vector<std::string> assert_vars_names;

//    	parsed_msg = parseErrorMsg(exe_err);

	std::string var1 = "", var2 = "";

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

        assert_vars_names.push_back(var1);
        assert_vars_names.push_back(var2);

//	assert_vars_names.push_back(parsed_msg.first);
//	assert_vars_names.push_back(parsed_msg.second);

	assert_vars = getApiObjects(assert_vars_names);

	
	for(std::vector<const ApiObject*>::iterator it = api_fuzzer->MVAR.begin(); it != api_fuzzer->MVAR.end(); it++)
	{
		mvar = *it;
		temp =  api_fuzzer->MetaVariant_Instr[mvar->getID()];

		res.push_back(temp.at(0));	

		for(int i = 0; i < indices.size(); i++)
		{
			res.push_back(temp.at(indices.at(i)));
		}
		
		if(var1 != "" && var2 != "")
		{
			if(assertReduction(temp.at(api_fuzzer->meta_test_count+1), assert_vars))
			{
				res.push_back(temp.at(api_fuzzer->meta_test_count+1));
			}
		}
		else
		{
			res.push_back(temp.at(api_fuzzer->meta_test_count+1));
		}
	}

	api_fuzzer->RED = res;
}


std::vector<const ApiObject*> Reduction::getApiObjects(std::vector<std::string> var)
{
        std::vector<const ApiObject*> all = api_fuzzer->meta_variants;

	std::vector<const ApiObject*> res;

	for(std::vector<const ApiObject*>::iterator it = all.begin(); it != all.end(); it++)
	{
		if(find(var.begin(), var.end(), (*it)->toStr()) != var.end())
		{
			res.push_back(*it);
		}
	}		

	return res;
}

bool Reduction::checkTestCase(std::string c_err, std::string n_c_err, std::string e_err, std::string n_e_err)
{
	std::pair<std::string,std::string> res1, res2;	
	
	if(c_err == n_c_err)
	{
//		res1 = parseErrorMsg(e_err);
//		res2 = parseErrorMsg(n_e_err);

//		if(res1.first == res2.first && res1.second == res2.second)
		if(e_err == n_e_err)
		{
			#if 0 
			std::cout << "Error Did Not Change" << std::endl;
			std::cout << "Old Compile error: " << c_err << std::endl;
			std::cout << "New Compile error: " << n_c_err << std::endl;

			std::cout << "Old Execute error: " << e_err << std::endl;
			std::cout << "New Execute error: " << n_e_err << std::endl;
			#endif

			return true;
		}
	}

	#if 0 
	std::cout << "Error Changed" << std::endl;
	std::cout << "Old Compile error: " << c_err << std::endl;
	std::cout << "New Compile error: " << n_c_err << std::endl;

	std::cout << "Old Execute error: " << e_err << std::endl;
	std::cout << "New Execute error: " << n_e_err << std::endl;
	#endif

	return false;
}

const ApiInstructionInterface* isPresent(std::vector<const ApiInstructionInterface*> red, const ApiInstructionInterface* instr)
{
	const ApiInstructionInterface* r = NULL;

	for(std::vector<const ApiInstructionInterface*>::iterator it = red.begin(); it != red.end(); it++)
	{
		if((*it)->toStr() == instr->toStr())
		{
			return *it;
		}
	}

	return r;
}
