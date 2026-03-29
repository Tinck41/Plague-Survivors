#pragma once

#include "flecs.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stdexcept>

namespace std {
	template<>
	struct hash<flecs::entity> {
		size_t operator()(const flecs::entity& e) const noexcept {
			return hash<flecs::entity_t>{}(e.id());
		}
	};
}

namespace ps {
	template<typename T>
	class Dag {
	public:
		bool add_edge(T from, T to) {
			if (from == to) { 
				return false;
			}

			if (would_create_cycle(from, to)) {
				return false;
			}

			adjacency[from].insert(to);
			in_degree[to]++;

			if (!in_degree.contains(from)) {
				in_degree[from] = 0;
			}

			return true;
		}

		void remove_edge(T from, T to) {
			auto it = adjacency.find(from);

			if (it == adjacency.end()) {
				return;
			}

			if (it->second.erase(to)) {
				in_degree[to]--;
			}
		}

		void remove_node(T node) {
			if (adjacency.contains(node)) {
				for (auto& to : adjacency[node]) {
					in_degree[to]--;
				}
				adjacency.erase(node);
			}

			for (auto& [from, neighbors] : adjacency) {
				if (neighbors.erase(node)) {
					in_degree[node]--;
				}
			}

			in_degree.erase(node);
		}

		std::vector<T> topological_sort() const {
			std::unordered_map<T, int> degrees = in_degree;

			std::vector<T> queue;
			for (auto& [node, degree] : degrees) {
				if (degree == 0) {
					queue.push_back(node);
				}
			}

			std::vector<T> result;

			while (!queue.empty()) {
				auto node = queue.back();
				queue.pop_back();
				result.push_back(node);

				if (!adjacency.contains(node)) {
					continue;
				}

				for (auto& neighbor : adjacency.at(node)) {
					if (--degrees[neighbor] == 0) {
						queue.push_back(neighbor);
					}
				}
			}

			return result;
		}

		const std::unordered_set<T>& neighbors(T node) const {
			static const std::unordered_set<T> empty;
			auto it = adjacency.find(node);
			return it != adjacency.end() ? it->second : empty;
		}

		bool has_node(T node) const {
			return in_degree.contains(node);
		}

		bool has_edge(T from, T to) const {
			auto it = adjacency.find(from);

			if (it == adjacency.end()) {
				return false;
			}

			return it->second.contains(to);
		}

	private:
		bool would_create_cycle(T from, T to) const {
			if (!adjacency.contains(to)) {
				return false;
			}

			std::unordered_set<T> visited;
			std::vector<T> stack{ to };

			while (!stack.empty()) {
				auto node = stack.back();
				stack.pop_back();

				if (node == from) {
					return true;
				}

				if (visited.contains(node)) {
					continue;
				}

				visited.insert(node);

				if (!adjacency.contains(node)) {
					continue;
				}

				for (auto& neighbor : adjacency.at(node)) {
					stack.push_back(neighbor);
				}
			}

			return false;
		}

		std::unordered_map<T, std::unordered_set<T>> adjacency;
		std::unordered_map<T, int> in_degree;
	};
}

