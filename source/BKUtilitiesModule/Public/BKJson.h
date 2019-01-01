// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKJson
#define Pragma_Once_BKJson

#include "BKEngine.h"
#include "BKString.h"
#include <string>
#include <vector>
#include <queue>
#include <iterator>
#include <istream>
#include <memory>

#define JSON_FIELD_NOT_FOUND FString(L"Not Found")

namespace BKJson
{
    class Node;
    typedef std::pair<FString, Node> NamedNode;

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
            T_BOOL,
            T_VALIDATION
        };

        Node();
        explicit Node(Type type);
        Node(const Node &other);
        Node(Type type, const FString &value);
        explicit Node(const FString &value);
        explicit Node(const UTFCHAR *value);
        explicit Node(int32 value);
        explicit Node(uint32 value);
        explicit Node(int64 value);
        explicit Node(uint64 value);
        explicit Node(float value);
        explicit Node(double value);
        explicit Node(bool value);
        ~Node();

        void Detach();

        inline Type GetType() const { return (!data ? T_INVALID : data->type); };

        inline bool IsValid()  const { return (GetType() != T_INVALID); }
        inline bool IsObject() const { return (GetType() == T_OBJECT);  }
        inline bool IsArray()  const { return (GetType() == T_ARRAY);   }
        inline bool IsNull()   const { return (GetType() == T_NULL);    }
        inline bool IsString() const { return (GetType() == T_STRING);  }
        inline bool IsNumber() const { return (GetType() == T_NUMBER);  }
        inline bool IsBoolean()   const { return (GetType() == T_BOOL);    }
        inline bool IsValidation() const { return (GetType() == T_VALIDATION);  }

        inline bool IsContainer() const { return (IsObject() || IsArray()); }
        inline bool IsValue() const { return (IsNull() || IsString() || IsNumber() || IsBoolean()); }

        FString ToString(const FString &def = FString(L"")) const;
        int32 ToInteger(int32 def = 0) const;
        float ToFloat(float def = 0.f) const;
        double ToDouble(double def = 0.0) const;
        bool ToBoolean(bool def = false) const;

        void SetNull();
        void Set(Type type, const FString &value);
        void Set(const FString &value);
        void Set(const UTFCHAR *value);
        void Set(int32 value);
        void Set(uint32 value);
        void Set(int64 value);
        void Set(uint64 value);
        void Set(float value);
        void Set(double value);
        void Set(bool value);

        Node &operator=(const Node &rhs);
        Node &operator=(const FString &rhs);
        Node &operator=(const UTFCHAR *rhs);
        Node &operator=(int32 rhs);
        Node &operator=(uint32 rhs);
        Node &operator=(int64 rhs);
        Node &operator=(uint64 rhs);
        Node &operator=(float rhs);
        Node &operator=(double rhs);
        Node &operator=(bool rhs);

        void Add(const Node &node);
        void Add(const FString &name, const Node &node);
        void Append(const Node &node);
        void Remove(size_t index);
        void Remove(const FString &name);
        void Clear();

        bool Has(const FString &name) const;
        size_t GetSize() const;
        Node Get(const FString &name) const;
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
            FString valueStr;
            NamedNodeList children;
        } *data;
    };

    FString EscapeString(const FString &value);
    FString UnescapeString(const FString &value);

    struct JsonFormatter
    {
        bool newline;
        bool spacing;
        bool useTabs;
        uint32 indentSize;
    };
    const JsonFormatter StandardFormat = { true, true, true, 1 };
    const JsonFormatter NoFormat = { false, false, false, 0 };

    class Writer
    {
    public:
        explicit Writer(const JsonFormatter &format = NoFormat);
        ~Writer();

        void SetFormat(const JsonFormatter &format);

        void WriteStream(const Node &node, FStringStream &stream) const;
        FString WriteString(const Node &node) const;

    private:
        void WriteNode(const Node &node, uint32 level, FStringStream &stream) const;
        void WriteObject(const Node &node, uint32 level, FStringStream &stream) const;
        void WriteArray(const Node &node, uint32 level, FStringStream &stream) const;
        void WriteValue(const Node &node, FStringStream &stream) const;

        FString GetIndentation(uint32 level) const;

        JsonFormatter format;
        UTFCHAR indentationChar;
        const UTFCHAR *newline;
        const UTFCHAR *spacing;
    };

    class JsonParser
    {
    public:
        JsonParser();
        ~JsonParser();

        Node ParseStream(std::wistream &stream);

        const FString &GetError() const;

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
        typedef std::queue<std::pair<Node::Type, FString> > DataQueue;

        void Tokenize(std::wistream &stream, TokenQueue &tokens, DataQueue &data);
        Node Assemble(TokenQueue &tokens, DataQueue &data);

        void JumpToNext(UTFCHAR c, std::wistream &stream);
        void JumpToCommentEnd(std::wistream &stream);

        void ReadString(std::wistream &stream, DataQueue &data);
        bool InterpretValue(const FString &value, DataQueue &data);

        FString error;
    };
}

#endif //Pragma_Once_BKJson