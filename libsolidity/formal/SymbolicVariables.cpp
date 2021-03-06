/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <libsolidity/formal/SymbolicVariables.h>

#include <libsolidity/formal/SymbolicTypes.h>
#include <libsolidity/ast/AST.h>

using namespace std;
using namespace solidity;
using namespace solidity::smtutil;
using namespace solidity::frontend;
using namespace solidity::frontend::smt;

SymbolicVariable::SymbolicVariable(
	TypePointer _type,
	TypePointer _originalType,
	string _uniqueName,
	EncodingContext& _context
):
	m_type(_type),
	m_originalType(_originalType),
	m_uniqueName(move(_uniqueName)),
	m_context(_context),
	m_ssa(make_unique<SSAVariable>())
{
	solAssert(m_type, "");
	m_sort = smtSort(*m_type);
	solAssert(m_sort, "");
}

SymbolicVariable::SymbolicVariable(
	SortPointer _sort,
	string _uniqueName,
	EncodingContext& _context
):
	m_sort(move(_sort)),
	m_uniqueName(move(_uniqueName)),
	m_context(_context),
	m_ssa(make_unique<SSAVariable>())
{
	solAssert(m_sort, "");
}

smtutil::Expression SymbolicVariable::currentValue(frontend::TypePointer const&) const
{
	return valueAtIndex(m_ssa->index());
}

string SymbolicVariable::currentName() const
{
	return uniqueSymbol(m_ssa->index());
}

smtutil::Expression SymbolicVariable::valueAtIndex(int _index) const
{
	return m_context.newVariable(uniqueSymbol(_index), m_sort);
}

string SymbolicVariable::nameAtIndex(int _index) const
{
	return uniqueSymbol(_index);
}

string SymbolicVariable::uniqueSymbol(unsigned _index) const
{
	return m_uniqueName + "_" + to_string(_index);
}

smtutil::Expression SymbolicVariable::resetIndex()
{
	m_ssa->resetIndex();
	return currentValue();
}

smtutil::Expression SymbolicVariable::setIndex(unsigned _index)
{
	m_ssa->setIndex(_index);
	return currentValue();
}

smtutil::Expression SymbolicVariable::increaseIndex()
{
	++(*m_ssa);
	return currentValue();
}

SymbolicBoolVariable::SymbolicBoolVariable(
	frontend::TypePointer _type,
	string _uniqueName,
	EncodingContext& _context
):
	SymbolicVariable(_type, _type, move(_uniqueName), _context)
{
	solAssert(m_type->category() == frontend::Type::Category::Bool, "");
}

SymbolicIntVariable::SymbolicIntVariable(
	frontend::TypePointer _type,
	frontend::TypePointer _originalType,
	string _uniqueName,
	EncodingContext& _context
):
	SymbolicVariable(_type, _originalType, move(_uniqueName), _context)
{
	solAssert(isNumber(m_type->category()), "");
}

SymbolicAddressVariable::SymbolicAddressVariable(
	string _uniqueName,
	EncodingContext& _context
):
	SymbolicIntVariable(TypeProvider::uint(160), TypeProvider::uint(160), move(_uniqueName), _context)
{
}

SymbolicFixedBytesVariable::SymbolicFixedBytesVariable(
	frontend::TypePointer _originalType,
	unsigned _numBytes,
	string _uniqueName,
	EncodingContext& _context
):
	SymbolicIntVariable(TypeProvider::uint(_numBytes * 8), _originalType, move(_uniqueName), _context)
{
}

SymbolicFunctionVariable::SymbolicFunctionVariable(
	frontend::TypePointer _type,
	string _uniqueName,
	EncodingContext& _context
):
	SymbolicVariable(_type, _type, move(_uniqueName), _context),
	m_declaration(m_context.newVariable(currentName(), m_sort))
{
	solAssert(m_type->category() == frontend::Type::Category::Function, "");
}

SymbolicFunctionVariable::SymbolicFunctionVariable(
	SortPointer _sort,
	string _uniqueName,
	EncodingContext& _context
):
	SymbolicVariable(move(_sort), move(_uniqueName), _context),
	m_declaration(m_context.newVariable(currentName(), m_sort))
{
	solAssert(m_sort->kind == Kind::Function, "");
}

smtutil::Expression SymbolicFunctionVariable::currentValue(frontend::TypePointer const& _targetType) const
{
	return m_abstract.currentValue(_targetType);
}

smtutil::Expression SymbolicFunctionVariable::currentFunctionValue() const
{
	return m_declaration;
}

smtutil::Expression SymbolicFunctionVariable::valueAtIndex(int _index) const
{
	return m_abstract.valueAtIndex(_index);
}

smtutil::Expression SymbolicFunctionVariable::functionValueAtIndex(int _index) const
{
	return SymbolicVariable::valueAtIndex(_index);
}

smtutil::Expression SymbolicFunctionVariable::resetIndex()
{
	SymbolicVariable::resetIndex();
	return m_abstract.resetIndex();
}

smtutil::Expression SymbolicFunctionVariable::setIndex(unsigned _index)
{
	SymbolicVariable::setIndex(_index);
	return m_abstract.setIndex(_index);
}

smtutil::Expression SymbolicFunctionVariable::increaseIndex()
{
	++(*m_ssa);
	resetDeclaration();
	m_abstract.increaseIndex();
	return m_abstract.currentValue();
}

smtutil::Expression SymbolicFunctionVariable::operator()(vector<smtutil::Expression> _arguments) const
{
	return m_declaration(_arguments);
}

void SymbolicFunctionVariable::resetDeclaration()
{
	m_declaration = m_context.newVariable(currentName(), m_sort);
}

SymbolicEnumVariable::SymbolicEnumVariable(
	frontend::TypePointer _type,
	string _uniqueName,
	EncodingContext& _context
):
	SymbolicVariable(_type, _type, move(_uniqueName), _context)
{
	solAssert(isEnum(m_type->category()), "");
}

SymbolicTupleVariable::SymbolicTupleVariable(
	frontend::TypePointer _type,
	string _uniqueName,
	EncodingContext& _context
):
	SymbolicVariable(_type, _type, move(_uniqueName), _context)
{
	solAssert(isTuple(m_type->category()), "");
}

SymbolicTupleVariable::SymbolicTupleVariable(
	SortPointer _sort,
	string _uniqueName,
	EncodingContext& _context
):
	SymbolicVariable(move(_sort), move(_uniqueName), _context)
{
	solAssert(m_sort->kind == Kind::Tuple, "");
}

vector<SortPointer> const& SymbolicTupleVariable::components()
{
	auto tupleSort = dynamic_pointer_cast<TupleSort>(m_sort);
	solAssert(tupleSort, "");
	return tupleSort->components;
}

smtutil::Expression SymbolicTupleVariable::component(
	size_t _index,
	TypePointer _fromType,
	TypePointer _toType
)
{
	optional<smtutil::Expression> conversion = symbolicTypeConversion(_fromType, _toType);
	if (conversion)
		return *conversion;

	return smtutil::Expression::tuple_get(currentValue(), _index);
}

SymbolicArrayVariable::SymbolicArrayVariable(
	frontend::TypePointer _type,
	frontend::TypePointer _originalType,
	string _uniqueName,
	EncodingContext& _context
):
	SymbolicVariable(_type, _originalType, move(_uniqueName), _context),
	m_pair(
		smtSort(*_type),
		m_uniqueName + "_length_pair",
		m_context
	)
{
	solAssert(isArray(m_type->category()) || isMapping(m_type->category()), "");
}

SymbolicArrayVariable::SymbolicArrayVariable(
	SortPointer _sort,
	string _uniqueName,
	EncodingContext& _context
):
	SymbolicVariable(move(_sort), move(_uniqueName), _context),
	m_pair(
		std::make_shared<TupleSort>(
			"array_length_pair",
			std::vector<std::string>{"array", "length"},
			std::vector<SortPointer>{m_sort, SortProvider::intSort}
		),
		m_uniqueName + "_array_length_pair",
		m_context
	)
{
	solAssert(m_sort->kind == Kind::Array, "");
}

smtutil::Expression SymbolicArrayVariable::currentValue(frontend::TypePointer const& _targetType) const
{
	optional<smtutil::Expression> conversion = symbolicTypeConversion(m_originalType, _targetType);
	if (conversion)
		return *conversion;

	return m_pair.currentValue();
}

smtutil::Expression SymbolicArrayVariable::valueAtIndex(int _index) const
{
	return m_pair.valueAtIndex(_index);
}

smtutil::Expression SymbolicArrayVariable::elements()
{
	return m_pair.component(0);
}

smtutil::Expression SymbolicArrayVariable::length()
{
	return m_pair.component(1);
}
