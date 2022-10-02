#pragma once

#include "ast.h"
#include "parser.h"
#include <unordered_set>
#include <vector>
#include <list>

namespace FrontEndPass
{

class DefaultRecur: public VisitorE
{
public:
    virtual void forNumber(const NumberE&) override {}
    virtual void forBoolean(const BooleanE&) override {}
    virtual void forVar(const Var&) override {}
    virtual void forQuote(const Quote&) override {}
    virtual void forDefine(const Define& def) override { def.body_->accept(*this); }
    virtual void forSetBang(const SetBang& setBang) override { setBang.e_->accept(*this); }
    virtual void forBegin(const Begin& bgn) override { for(const auto& e: bgn.es_) e->accept(*this); }
    virtual void forIf(const If& ifExpr) override { 
		ifExpr.pred_->accept(*this);
		ifExpr.thn_->accept(*this);
		ifExpr.els_->accept(*this);
	}
	virtual void forLet(const Let& let) override {
		for(const auto& kv: let.binds_) {
			kv.second->accept(*this);
		}
		let.body_->accept(*this);
	}

    virtual void forLambda(const Lambda& lam) override { lam.body_->accept(*this); }

    virtual void forApply(const Apply& app) override {
		app.operator_->accept(*this);
		for(const auto& arg: app.operands_) {
			arg->accept(*this);
		}
	}

};

/*
template<class FoldFn>
struct FoldExpr : public VisitorE 
{
    virtual void forNumber(const NumberE&){}
    virtual void forBoolean(const BooleanE&){}
    virtual void forVar(const Var& v){  }
    virtual void forQuote(const Quote&){}
    virtual void forDefine(const Define&){}
    virtual void forSetBang(const SetBang&){}
    virtual void forBegin(const Begin&){}
    virtual void forIf(const If&){}
	virtual void forLet(const Let&){}
    virtual void forLambda(const Lambda&){}
    virtual void forApply(const Apply&){}

	FoldFn fn;
};

*/


//store some info/data through some passes
struct PassContext
{
	std::unordered_set<std::string> assignedVars; //result from CollectAssign
	std::unordered_set<std::string> freeVars; //result from  FreeVariable
};

class Program
{
public:
	using Pair = std::pair<Define, PassContext>;
	Program(std::vector<std::unique_ptr<Define>> &&defs) { 
		for(auto& def: defs) {
			defs_.emplace_back(std::move(*def), PassContext{});
		}
	}

	auto begin() { return defs_.begin(); }
	auto end() { return defs_.end(); }

private:
	std::list<Pair> defs_;
};

class CollectAssign: public DefaultRecur
{
public:
    void forSetBang(const SetBang& setBang) override { assigned.insert(setBang.v_); }

	static void run(Program& prog, PassContext& ctx) {
		size_t i = 0;
		for(auto& def: prog) {
			CollectAssign collector;
			def.first.body_->accept(collector);
			ctx.assignedVars = std::move(collector.assigned);
		}
	}

private:
	std::unordered_set<std::string> assigned;
};

class FreeVariable: public DefaultRecur
{
public:
    virtual void forVar(const Var& v) override { freeVars_.insert(v.v_); };
	virtual void forLet(const Let& let) override { 
		let.body_->accept(*this);
		for(const auto& kv: let.binds_) {
			kv.second->accept(*this);
			freeVars_.erase(kv.first.v_);
		}
	}
    virtual void forLambda(const Lambda& lam) override {
		lam.body_->accept(*this);
		for(const auto& p: *lam.params_) {
			freeVars_.erase(p.v_);
		}
	}

	static void run(Program& prog, PassContext& ctx) {
		size_t i = 0;
		for(auto& def: prog) {
			FreeVariable fv;
			def.first.body_->accept(fv);
			ctx.freeVars = std::move(fv.freeVars_);
		}
	}
	
private:
	std::unordered_set<std::string> freeVars_;
};

//functions 
using PassFunc = void(*)(Program&);

Parser::Result<std::unique_ptr<Program>> parseProg(const Parser::Range& rg);


void  runAllPass(Program& p);

}//namespace FrontEndPass