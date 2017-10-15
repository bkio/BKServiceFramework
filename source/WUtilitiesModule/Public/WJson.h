// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WJson
#define Pragma_Once_WJson

#include "WEngine.h"

#include <string>
#include <vector>
#include <queue>
#include <iterator>
#include <istream>
#include <ostream>
#include <memory>

namespace WJson
{
    class Node;
    typedef std::pair<std::string, Node> NamedNode;

    class Node
    {
    public:
        class iterator : public std::iterator<std::input_iterator_tag, NamedNode>
        {
        public:
            iterator() : p(nullptr) {}
            explicit iterator(NamedNode *o) : p(o) {}
            iterator(const iterator &it) : p(it.p) {}

            iterator &operator++() { ++p; return *this; }
            iterator operator++(int) { iterator tmp(*this); operator++(); return tmp; }

            bool operator==(const iterator &rhs) { return p == rhs.p; }
            bool operator!=(const iterator &rhs) { return p != rhs.p; }

            NamedNode &operator*() { return *p; }
            NamedNode *operator->() { return p; }

        private:
            NamedNode *p;
        };
        class const_iterator : public std::iterator<std::input_iterator_tag, const NamedNode>
        {
        public:
            const_iterator() : p(nullptr) {}
            explicit const_iterator(const NamedNode *o) : p(o) {}
            const_iterator(const const_iterator &it) : p(it.p) {}

            const_iterator &operator++() { ++p; return *this; }
            const_iterator operator++(int) { const_iterator tmp(*this); operator++(); return tmp; }

            bool operator==(const const_iterator &rhs) { return p == rhs.p; }
            bool operator!=(const const_iterator &rhs) { return p != rhs.p; }

            const NamedNode &operator*() { return *p; }
            const NamedNode *operator->() { return p; }

        private:
            const NamedNode *p;
        };

        enum Type
        {
            T_INVALID,
            T_OBJECT,
            T_ARRAY,
            T_NULL,
            T_STRING,
            T_NUMBER,
            T_BOOL
        };

        Node();
        explicit Node(Type type);
        Node(const Node &other);
        Node(Type type, const std::string &value);
        explicit Node(const std::string &value);
        explicit Node(const ANSICHAR *value);
        explicit Node(int32 value);
        explicit Node(uint32 value);
        explicit Node(int64 value);
        explicit Node(uint64 value);
        explicit Node(float value);
        explicit Node(double value);
        explicit Node(bool value);
        ~Node();

        void Detach();

        inline Type GetType() const { return (data == nullptr ? T_INVALID : data->type); };

        inline bool IsValid()  const { return (GetType() != T_INVALID); }
        inline bool IsObject() const { return (GetType() == T_OBJECT);  }
        inline bool IsArray()  const { return (GetType() == T_ARRAY);   }
        inline bool IsNull()   const { return (GetType() == T_NULL);    }
        inline bool IsString() const { return (GetType() == T_STRING);  }
        inline bool IsNumber() const { return (GetType() == T_NUMBER);  }
        inline bool IsBoolean()   const { return (GetType() == T_BOOL);    }

        inline bool IsContainer() const { return (IsObject() || IsArray()); }
        inline bool IsValue() const { return (IsNull() || IsString() || IsNumber() || IsBoolean()); }

        std::string ToString(const std::string &def = std::string()) const;
        int32 ToInteger(int32 def = 0) const;
        float ToFloat(float def = 0.f) const;
        double ToDouble(double def = 0.0) const;
        bool ToBoolean(bool def = false) const;

        void SetNull();
        void Set(Type type, const std::string &value);
        void Set(const std::string &value);
        void Set(const ANSICHAR *value);
        void Set(int32 value);
        void Set(uint32 value);
        void Set(int64 value);
        void Set(uint64 value);
        void Set(float value);
        void Set(double value);
        void Set(bool value);

        Node &operator=(const Node &rhs);
        Node &operator=(const std::string &rhs);
        Node &operator=(const ANSICHAR *rhs);
        Node &operator=(int32 rhs);
        Node &operator=(uint32 rhs);
        Node &operator=(int64 rhs);
        Node &operator=(uint64 rhs);
        Node &operator=(float rhs);
        Node &operator=(double rhs);
        Node &operator=(bool rhs);

        void Add(const Node &node);
        void Add(const std::string &name, const Node &node);
        void Append(const Node &node);
        void Remove(size_t index);
        void Remove(const std::string &name);
        void Clear();

        bool Has(const std::string &name) const;
        size_t GetSize() const;
        Node Get(const std::string &name) const;
        Node Get(size_t index) const;

        iterator begin();
        const_iterator begin() const;
        iterator end();
        const_iterator end() const;

        bool operator==(const Node &other) const;
        bool operator!=(const Node &other) const;
        inline explicit operator bool() const { return IsValid(); }

    private:
        typedef std::vector<NamedNode> NamedNodeList;
        struct Data
        {
            explicit Data(Type type);
            Data(const Data &other);
            ~Data();
            void addRef();
            bool release();
            int32 refCount;

            Type type;
            std::string valueStr;
            NamedNodeList children;
        } *data;
    };

    std::string EscapeString(const std::string &value);
    std::string UnescapeString(const std::string &value);

    Node Invalid();
    Node Null();
    Node Object();
    Node Array();

    struct Format
    {
        bool newline;
        bool spacing;
        bool useTabs;
        uint32 indentSize;
    };
    const Format StandardFormat = { true, true, true, 1 };
    const Format NoFormat = { false, false, false, 0 };

    class Writer
    {
    public:
        explicit Writer(const Format &format = NoFormat);
        ~Writer();

        void SetFormat(const Format &format);

        void WriteStream(const Node &node, std::ostream &stream) const;
        void WriteString(const Node &node, std::string &json) const;
        void WriteFile(const Node &node, const std::string &filename) const;

    private:
        void WriteNode(const Node &node, uint32 level, std::ostream &stream) const;
        void WriteObject(const Node &node, uint32 level, std::ostream &stream) const;
        void WriteArray(const Node &node, uint32 level, std::ostream &stream) const;
        void WriteValue(const Node &node, std::ostream &stream) const;

        std::string GetIndentation(uint32 level) const;

        Format format;
        ANSICHAR indentationChar;
        const ANSICHAR *newline;
        const ANSICHAR *spacing;
    };

    class Parser
    {
    public:
        Parser();
        ~Parser();

        Node ParseStream(std::istream &stream);
        Node ParseString(const std::string &json);
        Node ParseFile(const std::string &filename);

        const std::string &GetError() const;

    private:
        enum Token
        {
            T_UNKNOWN,
            T_OBJ_BEGIN,
            T_OBJ_END,
            T_ARRAY_BEGIN,
            T_ARRAY_END,
            T_SEPARATOR_NODE,
            T_SEPARATOR_NAME,
            T_VALUE
        };
        typedef std::queue<Token> TokenQueue;
        typedef std::queue<std::pair<Node::Type, std::string> > DataQueue;

        void Tokenize(std::istream &stream, TokenQueue &tokens, DataQueue &data);
        Node Assemble(TokenQueue &tokens, DataQueue &data);

        void JumpToNext(ANSICHAR c, std::istream &stream);
        void JumpToCommentEnd(std::istream &stream);

        void ReadString(std::istream &stream, DataQueue &data);
        bool InterpretValue(const std::string &value, DataQueue &data);

        std::string error;
    };
    Node Invalid()
    {
        return Node(Node::T_INVALID);
    }
    std::shared_ptr<Node> InvalidPtr()
    {
        return std::make_shared<Node>(Node::T_INVALID);
    }
    Node Null()
    {
        return Node(Node::T_NULL);
    }
    std::shared_ptr<Node> NullPtr()
    {
        return std::make_shared<Node>(Node::T_NULL);
    }
    Node Object()
    {
        return Node(Node::T_OBJECT);
    }
    std::shared_ptr<Node> ObjectPtr()
    {
        return std::make_shared<Node>(Node::T_OBJECT);
    }
    Node Array()
    {
        return Node(Node::T_ARRAY);
    }
    std::shared_ptr<Node> ArrayPtr()
    {
        return std::make_shared<Node>(Node::T_ARRAY);
    }
}
#define NULL_WJSON_NODE std::shared_ptr<WJson::Node>(nullptr)

#endif //Pragma_Once_WJson