#pragma once

#include <type_traits>
#include <variant>

namespace se {
	template<class... Ts>
	struct visitors : Ts... { using Ts::operator()...; };

	template<class... Ts>
	visitors(Ts...) -> visitors<Ts...>;
	 
	template<typename Value, typename Visitors>
	decltype(auto) visit(Value&& value, Visitors&& visitors) {
		return std::visit(std::forward<Visitors>(visitors), std::forward<Value>(value));
	}
}
