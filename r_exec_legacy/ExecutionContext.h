#ifndef __EXECUTION_CONTEXT_H
#define __EXECUTION_CONTEXT_H
#include "Expression.h"
#include "ReductionInstance.h"
#include "opcodes.h"

namespace r_exec {

class ExecutionContext : public Expression {
public:
	ExecutionContext(Expression& e) :Expression(e) {}
	Expression evaluateOperand(int index);
	Expression evaluate();
	ExecutionContext xchild(int index) const;

	void setResult(r_code::Atom result);
	void setResultTimestamp(int64 timestamp);
	void beginResultSet();
	void appendResultSetElement(r_code::Atom element);
	void endResultSet();
	void undefinedResult();

	void pushResultAtom(r_code::Atom a);
	Expression getEndExpression() const;
private:
	std::vector<r_code::Atom> resultSet;
};
inline void ExecutionContext::setResult(r_code::Atom result) { instance->value[index] = result; }
}

#endif // __EXECUTION_CONTEXT_H