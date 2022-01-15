#include <string>
#include <stack>
#include <variant>
#include <optional>

#include "json.h"

namespace json {
	class Builder;
	class DictItemContext;
	class KeyItemContext;
	class ArrayItemContext;

	class StartArrayDictContext {
	public:
		StartArrayDictContext(Builder&);

		DictItemContext StartDict();

		ArrayItemContext StartArray();
	private:
		Builder& builder_;
	};

	class KeyItemContext final : public StartArrayDictContext{
	public:
		KeyItemContext(Builder&);

		DictItemContext Value(Node::Value);
	private:
		Builder& builder_;
	};

	class ArrayItemContext final : public StartArrayDictContext{
	public:
		ArrayItemContext(Builder&);

		ArrayItemContext Value(Node::Value);

		Builder& EndArray();
	private:
		Builder& builder_;
	};

	class DictItemContext final {
	public:
		DictItemContext(Builder&);

		KeyItemContext Key(std::string);

		Builder& EndDict();
	private:
		Builder& builder_;
	};


	class Builder {
	public:
		KeyItemContext Key(std::string);

		Builder& Value(Node::Value);

		DictItemContext StartDict();
		Builder& EndDict();

		ArrayItemContext StartArray();
		Builder& EndArray();

		json::Node Build();
	private:
		std::optional<Node> root_ = std::nullopt;
		std::vector<Node*> nodes_stack;
	};
}