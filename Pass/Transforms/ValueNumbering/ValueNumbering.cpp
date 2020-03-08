#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
using namespace llvm;
using namespace std;

#define DEBUG_TYPE "ValueNumbering"

using namespace llvm;

namespace {
struct ValueNumbering : public FunctionPass {
    string func_name = "test";
    static char ID;
    ValueNumbering() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        if (F.getName() != func_name)
            return false;
        std::map<BasicBlock *, std::set<std::string>> GlobalUE;
        std::map<BasicBlock *, std::set<std::string>>::iterator globalue;
        std::map<BasicBlock *, std::set<std::string>> GlobalKill;
        std::map<BasicBlock *, std::set<std::string>>::iterator globalkill;

        for (auto &basic_block : F) {
            std::set<std::string> UEVar;
            std::set<std::string> VarKill;
            GlobalUE.insert(std::pair<BasicBlock *, std::set<std::string>>(
                &basic_block, UEVar));
            GlobalKill.insert(std::pair<BasicBlock *, std::set<std::string>>(
                &basic_block, VarKill));
            // errs() << "----- " << basic_block.getName() << " -----\n";
            for (auto &inst : basic_block) {
                if (!(inst.getOpcode() == Instruction::Load ||
                      inst.getOpcode() == Instruction::Store))
                    continue;
                // errs() << inst << '\n';
                // errs() << "# of operands: " << inst.getNumOperands() << '\n';
                // errs() << inst.getOpcode() << '\n';
                // https://llvm.org/doxygen/inst_8cpp_source.html
                std::string Variable;
                if (inst.getOpcode() == Instruction::Load)
                    Variable = inst.getOperand(0)->getName();
                else
                    Variable = inst.getOperand(1)->getName();
                // errs() << inst.getOperand(0)->getName() <<
                // "~~~"<<inst.getOperand(1)->getName()<<"\n";
                if (inst.getOpcode() == Instruction::Store) {
                    globalkill = GlobalKill.find(&basic_block);
                    VarKill = globalkill->second;
                    // errs()<<"killed:"<<Variable<<"\n";
                    VarKill.insert(Variable);
                    GlobalKill.erase(globalkill);
                    GlobalKill.insert(
                        std::pair<BasicBlock *, std::set<std::string>>(
                            &basic_block, VarKill));
                    continue;
                }
                globalkill = GlobalKill.find(&basic_block);
                if (globalkill->second.find(Variable) !=
                    globalkill->second.end())
                    continue;
                // errs() << "not killed\n";
                globalue = GlobalUE.find(&basic_block);
                UEVar = globalue->second;
                if (inst.getOpcode() == 31) {
                    // errs()<<"insert:"<<Variable<<"\n";
                    UEVar.insert(Variable);
                }
                GlobalUE.erase(globalue);
                GlobalUE.insert(std::pair<BasicBlock *, std::set<std::string>>(
                    &basic_block, UEVar));
            }
        }

        std::map<BasicBlock *, std::set<std::string>> LiveOut;
        std::map<BasicBlock *, std::set<std::string>>::iterator liveout;
        std::list<BasicBlock*> WorkList;
        for (auto &basic_block : F) {
            std::set<std::string> temp;
            LiveOut.insert(std::pair<BasicBlock *, std::set<std::string>>(
                &basic_block, temp));
            int count = 0;
            for (BasicBlock *Succ : successors(&basic_block)) {
                count++;
            }
            if (!count)
                WorkList.push_back(&basic_block);
        }
        while (!WorkList.empty() ) { 
            auto *basic_block = WorkList.front();
            WorkList.pop_front();
            
            for (BasicBlock *Pred : predecessors(basic_block)) {
                std::set<std::string> UEVar;
                std::set<std::string> VarKill;
                std::set<std::string> Live;
                std::set<std::string> temp;
                std::vector<std::string> temp1;
                std::vector<std::string> temp2;;
                std::vector<std::string> result;
                
                Live = LiveOut.find(basic_block)->second;
                UEVar = GlobalUE.find(basic_block)->second;
                VarKill = GlobalKill.find(basic_block)->second;
                temp = LiveOut.find(Pred)->second;
                std::vector<std::string> compare(temp.size());
                std::copy(temp.begin(), temp.end(), compare.begin());
                std::set_difference(Live.begin(), Live.end(), VarKill.begin(), VarKill.end(),std::back_inserter(temp1));
                std::set_union(temp1.begin(), temp1.end(), UEVar.begin(), UEVar.end(),std::back_inserter(temp2));
                std::set_union(temp2.begin(), temp2.end(), compare.begin(), compare.end(),std::back_inserter(result));
                
                if (result.size()>compare.size()){
                    WorkList.push_back(Pred);
                    std::set<std::string> temp4(result.begin(), result.end());
                    std::pair <BasicBlock*, std::set<std::string>> Pair (Pred, temp4);              
                    liveout = LiveOut.find(Pred);
                    LiveOut.erase(liveout);
                    LiveOut.insert(std::pair<BasicBlock*, std::set<std::string>>(Pred,temp4));
                }
            }
        }
        for (auto &basic_block : F) {
            errs() << "----- " << basic_block.getName() << " -----\n";

            errs() << "UEVAR:";
            globalue = GlobalUE.find(&basic_block);
            std::set<std::string> UEVar = globalue->second;
            for (std::set<std::string>::iterator it = UEVar.begin();
                 it != UEVar.end(); it++)
                errs() << " " << *it;
            errs() << "\n";

            errs() << "VARKILL:";
            globalkill = GlobalKill.find(&basic_block);
            std::set<std::string> VarKill = globalkill->second;
            for (std::set<std::string>::iterator it = VarKill.begin();
                 it != VarKill.end(); it++)
                errs() << " " << *it;
            errs() << "\n";

            errs() << "LIVEOUT:";
            liveout = LiveOut.find(&basic_block);
            std::set<std::string> Live = liveout->second;
            for (std::set<std::string>::iterator it = Live.begin();
                 it != Live.end(); it++)
                errs() << " " << *it;
            errs() << "\n";
        }
        return false;
    }
}; // end of struct ValueNumbering
} // end of anonymous namespace

char ValueNumbering::ID = 0;
static RegisterPass<ValueNumbering> X("ValueNumbering", "ValueNumbering Pass",
                                      false /* Only looks at CFG */,
                                      false /* Analysis Pass */);