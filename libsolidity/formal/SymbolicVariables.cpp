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

smt::Expression SymbolicVariable::currentValue(frontend::TypePointer const&) const
{
	return valueAtIndex(m_ssa->index());
}

string SymbolicVariable::currentName() const
{
	return uniqueSymbol(m_ssa->index());
}

smt::Expression SymbolicVariable::valueAtIndex(int _index) const
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

smt::Expression SymbolicVariable::resetIndex()
{
	m_ssa->resetIndex();
	return currentValue();
}

smt::Expression SymbolicVariable::increaseIndex()
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

smt::Expression SymbolicFunctionVariable::currentValue(frontend::TypePointer const& _targetType) const
{
	return m_abstract.currentValue(_targetType);
}

smt::Expression SymbolicFunctionVariable::currentFunctionValue() const
{
	return m_declaration;
}

smt::Expression SymbolicFunctionVariable::valueAtIndex(int _index) const
{
	return m_abstract.valueAtIndex(_index);
}

smt::Expression SymbolicFunctionVariable::functionValueAtIndex(int _index) const
{
	return SymbolicVariable::valueAtIndex(_index);
}

smt::Expression SymbolicFunctionVariable::resetIndex()
{
	SymbolicVariable::resetIndex();
	return m_abstract.resetIndex();
}

smt::Expression SymbolicFunctionVariable::increaseIndex()
{
	++(*m_ssa);
	resetDeclaration();
	m_abstract.increaseIndex();
	return m_abstract.currentValue();
}

smt::Expression SymbolicFunctionVariable::operator()(vector<smt::Expression> _arguments) const
{
	return m_declaration(_arguments);
}

void SymbolicFunctionVariable::resetDeclaration()
{
	m_declaration = m_context.newVariable(currentName(), m_sort);
}

SymbolicMappingVariable::SymbolicMappingVariable(
	frontend::TypePointer _type,
	string _uniqueName,
	EncodingContext& _context
):
	SymbolicVariable(_type, _type, move(_uniqueName), _context)
{
	solAssert(isMapping(m_type->category()), "");
}

SymbolicArrayVariable::SymbolicArrayVariable(
	frontend::TypePointer _type,
	frontend::TypePointer _originalType,
	string _uniqueName,
	EncodingContext& _context
):
	SymbolicVariable(_type, _originalType, move(_uniqueName), _context)
{
	solAssert(isArray(m_type->category()), "");
}

smt::Expression SymbolicArrayVariable::currentValue(frontend::TypePointer const& _targetType) const
{
	if (_targetType)
		// StringLiterals are encoded as SMT arrays in the generic case,
		// but they can also be compared/assigned to fixed bytes, in which
		// case they'd need to be encoded as numbers.
		if (auto strType = dynamic_cast<StringLiteralType const*>(m_originalType))
			if (_targetType->category() == frontend::Type::Category::FixedBytes)
				return smt::Expression(u256(toHex(util::asBytes(strType->value()), util::HexPrefix::Add)));

	return SymbolicVariable::currentValue(_targetType);
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
	auto const& tupleType = dynamic_cast<TupleType const&>(*m_type);
	auto const& componentsTypes = tupleType.components();
	for (unsigned i = 0; i < componentsTypes.size(); ++i)
		if (componentsTypes.at(i))
		{
			string componentName = m_uniqueName + "_component_" + to_string(i);
			auto result = smt::newSymbolicVariable(*componentsTypes.at(i), componentName, m_context);
			solAssert(result.second, "");
			m_components.emplace_back(move(result.second));
		}
		else
			m_components.emplace_back(nullptr);
}
