#include "json_builder.h"

#include <utility>

namespace json {

	KeyItemContext json::Builder::Key(std::string key) {
		nodes_stack.push_back(new Node{ key });
		return *this;
	}

	Builder& json::Builder::Value(Node::Value value) {
		if (!root_.has_value()) {
			root_.emplace(value);
		}
		else if (!nodes_stack.empty() && nodes_stack.back()->IsString()) {
			std::string key = nodes_stack.back()->AsString();
			nodes_stack.pop_back();
			Dict dict(nodes_stack.back()->AsDict());
			dict.emplace(key, value);
			*nodes_stack.back() = move(dict);
		}
		else if (!nodes_stack.empty() && nodes_stack.back()->IsArray()) {
			Array arr(nodes_stack.back()->AsArray());
			arr.emplace_back(value);
			*nodes_stack.back() = move(arr);
		}
		return *this;
	}

	DictItemContext json::Builder::StartDict() {
		if (!root_.has_value()) {
			root_.emplace(Dict{});
			nodes_stack.push_back(&(*root_));
		}
		else if (!nodes_stack.empty() && nodes_stack.back()->IsArray()) {
			Array arr(nodes_stack.back()->AsArray());
			arr.push_back(Node(Dict{}));
			Node* node = &arr.back();
			*nodes_stack.back() = move(arr);
			nodes_stack.push_back(node);
		}
		else if (!nodes_stack.empty() && nodes_stack.back()->IsString()) {
			std::string key = nodes_stack.back()->AsString();
			nodes_stack.pop_back();
			Dict dict(nodes_stack.back()->AsDict());
			Node* node = &(*dict.emplace(key, Dict{}).first).second;
			*nodes_stack.back() = move(dict);
			nodes_stack.emplace_back(node);
		}
		return *this;
	}

	Builder& json::Builder::EndDict() {
		if (!nodes_stack.empty() && nodes_stack.back()->IsDict()) {
			nodes_stack.pop_back();
		}
		else {
			throw std::logic_error("Ivalid usage of EndDict()");
		}
		return *this;
	}

	ArrayItemContext json::Builder::StartArray() {
		if (!root_.has_value()) {
			root_.emplace(Array{});
			nodes_stack.push_back(&(*root_));
		}
		else if (!nodes_stack.empty() && nodes_stack.back()->IsArray()) {
			Array arr(nodes_stack.back()->AsArray());
			arr.push_back(Node(Array{}));
			Node* node = &arr.back();
			*nodes_stack.back() = move(arr);
			nodes_stack.push_back(node);
		}
		else if (!nodes_stack.empty() && nodes_stack.back()->IsString()) {
			std::string key = nodes_stack.back()->AsString();
			nodes_stack.pop_back();
			Dict dict(nodes_stack.back()->AsDict());
			Node* node = &(*dict.emplace(key, Array{}).first).second;
			*nodes_stack.back() = move(dict);
			nodes_stack.emplace_back(node);
		}
		return *this;
	}

	Builder& json::Builder::EndArray() {
		if (!nodes_stack.empty() && nodes_stack.back()->IsArray()) {
			nodes_stack.pop_back();
		}
		else {
			throw std::logic_error("Invalid usage of EndArray()");
		}
		return *this;
	}

	json::Node Builder::Build() {
		if (!root_.has_value() || !nodes_stack.empty()) {
			throw std::logic_error("Invalid Build");
		}
		return *root_;
	}

	// ---- DICT ITEM CONTEXT -----
	DictItemContext::DictItemContext(Builder& builder)
		: builder_(builder){
	}

	KeyItemContext DictItemContext::Key(std::string key) {
		return builder_.Key(key);
	}
	Builder& DictItemContext::EndDict() {
		return builder_.EndDict();
	}

	// ----- StartArrayDictContext ---
	StartArrayDictContext::StartArrayDictContext(Builder& builder)
		: builder_(builder){
	}

	DictItemContext StartArrayDictContext::StartDict() {
		return builder_.StartDict();
	}

	ArrayItemContext StartArrayDictContext::StartArray() {
		return builder_.StartArray();
	}

	// ----- KeyItemContext ------
	KeyItemContext::KeyItemContext(Builder& builder)
		: StartArrayDictContext(builder), builder_(builder) {
	}

	DictItemContext KeyItemContext::Value(Node::Value value) {
		return builder_.Value(value);
	}

	// ---- ArrayItemContext ----
	ArrayItemContext::ArrayItemContext(Builder& builder)
		: StartArrayDictContext(builder), builder_(builder) {
	}

	ArrayItemContext ArrayItemContext::Value(Node::Value value) {
		return builder_.Value(value);
	}

	Builder& ArrayItemContext::EndArray() {
		return builder_.EndArray();
	}
}