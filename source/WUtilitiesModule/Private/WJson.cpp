// Copyright Pagansoft.com, All rights reserved.

#include "WJson.h"
#include <fstream>
#include <cassert>

namespace WJson
{
    namespace
    {
        inline bool isWhitespace(ANSICHAR c)
        {
            return (c == '\n' || c == ' ' || c == '\t' || c == '\r' || c == '\f');
        }

        const ANSICHAR charsUnescaped[] = { '\\'  , '/'  , '\"'  , '\n' , '\t' , '\b' , '\f' , '\r' };
        const ANSICHAR *charsEscaped[]  = { "\\\\", "\\/", "\\\"", "\\n", "\\t", "\\b", "\\f", "\\r" };
        const uint32 numEscapeChars = 8;
        const ANSICHAR nullUnescaped = '\0';
        const ANSICHAR *nullEscaped  = "\0\0";
        const ANSICHAR *getEscaped(const ANSICHAR c)
        {
            for (uint32 i = 0; i < numEscapeChars; ++i)
            {
                const ANSICHAR &ue = charsUnescaped[i];

                if (c == ue)
                {
                    return charsEscaped[i];
                }
            }
            return nullEscaped;
        }
        ANSICHAR getUnescaped(const ANSICHAR c1, const ANSICHAR c2)
        {
            for (uint32 i = 0; i < numEscapeChars; ++i)
            {
                const ANSICHAR *e = charsEscaped[i];

                if (c1 == e[0] && c2 == e[1])
                {
                    return charsUnescaped[i];
                }
            }
            return nullUnescaped;
        }
    }

    Node::Node() : data(nullptr)
    {
    }
    Node::Node(Type type)
    {
        if (type != T_INVALID)
            data = new Data(type);
        else
            data = nullptr;
    }
    Node::Node(const Node &other) : data(other.data)
    {
        if (data)
        {
            data->addRef();
        }
    }
    Node::Node(Type type, const FString &value) : data(new Data(T_NULL)) { Set(type, value); }
    Node::Node(const FString &value) : data(new Data(T_STRING)) { Set(value); }
    Node::Node(const ANSICHAR *value) : data(new Data(T_STRING)) { Set(value); }
    Node::Node(int32 value) : data(new Data(T_NUMBER)) { Set(value); }
    Node::Node(uint32 value) : data(new Data(T_NUMBER)) { Set(value); }
    Node::Node(int64 value) : data(new Data(T_NUMBER)) { Set(value); }
    Node::Node(uint64 value) : data(new Data(T_NUMBER)) { Set(value); }
    Node::Node(float value) : data(new Data(T_NUMBER)) { Set(value); }
    Node::Node(double value) : data(new Data(T_NUMBER)) { Set(value); }
    Node::Node(bool value) : data(new Data(T_BOOL)) { Set(value); }
    Node::~Node()
    {
        if (data && data->release())
        {
            delete data;
            data = nullptr;
        }
    }

    void Node::Detach()
    {
        if (data && data->refCount > 1)
        {
            auto newData = new Data(*data);
            if (data->release())
            {
                delete data;
            }
            data = newData;
        }
    }

    FString Node::ToString(const FString &def) const
    {
        if (IsValue())
        {
            if (IsNull())
            {
                return FString("null");
            }
            else if (IsValidation())
            {
                return FString("validation");
            }
            else
            {
                return data->valueStr;
            }
        }
        else
        {
            return def;
        }
    }
#define GET_NUMBER(T) \
	if (IsNumber())\
	{\
		FStringStream sstr(data->valueStr);\
		T val;\
		sstr >> val;\
		return val;\
	}\
	else\
	{\
		return def;\
	}
    int32 Node::ToInteger(int32 def) const { GET_NUMBER(int) }
    float Node::ToFloat(float def) const { GET_NUMBER(float) }
    double Node::ToDouble(double def) const { GET_NUMBER(double) }
#undef GET_NUMBER
    bool Node::ToBoolean(bool def) const
    {
        if (IsBoolean())
        {
            return (data->valueStr == FString("true"));
        }
        else
        {
            return def;
        }
    }

    void Node::SetNull()
    {
        if (IsValue())
        {
            Detach();
            data->type = T_NULL;
            data->valueStr.Empty();
        }
    }
    void Node::Set(Type type, const FString &value)
    {
        if (IsValue() && (type == T_NULL || type == T_STRING || type == T_NUMBER || type == T_BOOL))
        {
            Detach();
            data->type = type;
            if (type == T_STRING)
            {
                data->valueStr = UnescapeString(value);
            }
            else
            {
                data->valueStr = value;
            }
        }
    }
    void Node::Set(const FString &value)
    {
        if (IsValue())
        {
            Detach();
            data->type = T_STRING;
            data->valueStr = UnescapeString(value);
        }
    }
    void Node::Set(const ANSICHAR *value)
    {
        if (IsValue())
        {
            Detach();
            data->type = T_STRING;
            data->valueStr = UnescapeString(FString(value));
        }
    }
#define SET_NUMBER \
	if (IsValue())\
	{\
		Detach();\
		data->type = T_NUMBER;\
		FStringStream sstr(false);\
		sstr << value;\
		data->valueStr = sstr.Str();\
	}
    void Node::Set(int32 value) { SET_NUMBER }
    void Node::Set(uint32 value) { SET_NUMBER }
    void Node::Set(int64 value) { SET_NUMBER }
    void Node::Set(uint64 value) { SET_NUMBER }
    void Node::Set(float value) { SET_NUMBER }
    void Node::Set(double value) { SET_NUMBER }
#undef SET_NUMBER
    void Node::Set(bool value)
    {
        if (IsValue())
        {
            Detach();
            data->type = T_BOOL;
            data->valueStr = (value ? "true" : "false");
        }
    }

    Node &Node::operator=(const Node &rhs)
    {
        if (this != &rhs)
        {
            if (data && data->release())
            {
                delete data;
            }
            data = rhs.data;
            if (data)
            {
                data->addRef();
            }
        }
        return *this;
    }
    Node &Node::operator=(const FString &rhs) { Set(rhs); return *this; }
    Node &Node::operator=(const ANSICHAR *rhs) { Set(rhs); return *this; }
    Node &Node::operator=(int32 rhs) { Set(rhs); return *this; }
    Node &Node::operator=(uint32 rhs) { Set(rhs); return *this; }
    Node &Node::operator=(int64 rhs) { Set(rhs); return *this; }
    Node &Node::operator=(uint64 rhs) { Set(rhs); return *this; }
    Node &Node::operator=(float rhs) { Set(rhs); return *this; }
    Node &Node::operator=(double rhs) { Set(rhs); return *this; }
    Node &Node::operator=(bool rhs) { Set(rhs); return *this; }

    void Node::Add(const Node &node)
    {
        if (IsArray())
        {
            Detach();
            data->children.emplace_back(FString(""), node);
        }
    }
    void Node::Add(const FString &name, const Node &node)
    {
        if (IsObject())
        {
            Detach();
            data->children.emplace_back(name, node);
        }
    }
    void Node::Append(const Node &node)
    {
        if ((IsObject() && node.IsObject()) || (IsArray() && node.IsArray()))
        {
            Detach();
            data->children.insert(data->children.end(), node.data->children.begin(), node.data->children.end());
        }
    }
    void Node::Remove(size_t index)
    {
        if (IsContainer() && index < data->children.size())
        {
            Detach();
            auto it = data->children.begin()+index;
            data->children.erase(it);
        }
    }
    void Node::Remove(const FString &name)
    {
        if (IsObject())
        {
            Detach();
            NamedNodeList &children = data->children;
            for (auto it = children.begin(); it != children.end(); ++it)
            {
                if ((*it).first == name)
                {
                    children.erase(it);
                    break;
                }
            }
        }
    }
    void Node::Clear()
    {
        if (data && !data->children.empty())
        {
            Detach();
            data->children.clear();
        }
    }

    bool Node::Has(const FString &name) const
    {
        if (IsObject())
        {
            NamedNodeList &children = data->children;
            for (NamedNodeList::const_iterator it = children.begin(); it != children.end(); ++it)
            {
                if (name.Equals(it->first))
                {
                    return true;
                }
            }
        }
        return false;
    }
    size_t Node::GetSize() const
    {
        if (IsObject())
        {
            std::vector<FString> Checked;

            NamedNodeList &children = data->children;
            for (NamedNodeList::const_iterator it = children.begin(); it != children.end(); ++it)
            {
                if(std::find(Checked.begin(), Checked.end(), (*it).first) == Checked.end())
                {
                    Checked.emplace_back((*it).first);
                }
            }
            return Checked.size();
        }
        if (IsArray())
        {
            return data ? data->children.size() : 0;
        }
        return 0;
    }
    Node Node::Get(const FString &name) const
    {
        if (IsObject())
        {
            NamedNodeList &children = data->children;
            for (NamedNodeList::const_iterator it = children.begin(); it != children.end(); ++it)
            {
                if (name.Equals(it->first))
                {
                    return it->second;
                }
            }
        }
        return Node(T_INVALID);
    }
    Node Node::Get(size_t index) const
    {
        if (IsContainer() && index < data->children.size())
        {
            return data->children.at(index).second;
        }
        return Node(T_INVALID);
    }

    Node::iterator Node::begin()
    {
        if (data && !data->children.empty())
            return Node::iterator(&data->children.front());
        else
            return Node::iterator(nullptr);
    }
    Node::const_iterator Node::begin() const
    {
        if (data && !data->children.empty())
            return Node::const_iterator(&data->children.front());
        else
            return Node::const_iterator(nullptr);
    }
    Node::iterator Node::end()
    {
        if (data && !data->children.empty())
            return Node::iterator(&data->children.back()+1);
        else
            return Node::iterator(nullptr);
    }
    Node::const_iterator Node::end() const
    {
        if (data && !data->children.empty())
            return Node::const_iterator(&data->children.back()+1);
        else
            return Node::const_iterator(nullptr);
    }

    bool Node::operator==(const Node &other) const
    {
        return (
                (data == other.data) ||
                (IsValue() && (data->type == other.data->type)&&(data->valueStr == other.data->valueStr)));
    }
    bool Node::operator!=(const Node &other) const
    {
        return !(*this == other);
    }

    Node::Data::Data(Type type) : refCount(1), type(type)
    {
    }
    Node::Data::Data(const Data &other) : refCount(1), type(other.type), valueStr(other.valueStr), children(other.children)
    {
    }
    Node::Data::~Data()
    {
        assert(refCount == 0);
    }
    void Node::Data::addRef()
    {
        ++refCount;
    }
    bool Node::Data::release()
    {
        return (--refCount == 0);
    }

    FString EscapeString(const FString &value)
    {
        FString escaped;
        escaped.Reserve(static_cast<uint32>(value.Len()));

        ANSICHAR c;
        for (uint32 i = 0; i < value.Len(); i++)
        {
            c = value.AtAnsi(i);

            const ANSICHAR *a = getEscaped(c);
            if (a[0] != '\0')
            {
                escaped += a[0];
                escaped += a[1];
            }
            else
            {
                escaped += c;
            }
        }

        return escaped;
    }
    FString UnescapeString(const FString &value)
    {
        FString unescaped;

        ANSICHAR c1, c2;
        for (uint32 i = 0; i < value.Len(); i++)
        {
            c1 = value.AtAnsi(i);
            if ((i + 1) != value.Len())
            {
                c2 = value.AtAnsi(i + 1);
                i++;
            }
            else
            {
                c2 = '\0';
            }

            const ANSICHAR a = getUnescaped(c1, c2);
            if (a != '\0')
            {
                unescaped += a;
            }
            else
            {
                unescaped += c1;
            }
        }
        return unescaped;
    }

    Writer::Writer(const JsonFormatter &format)
    {
        SetFormat(format);
    }
    Writer::~Writer() = default;

    void Writer::SetFormat(const JsonFormatter &format)
    {
        this->format = format;
        indentationChar = static_cast<ANSICHAR>(format.useTabs ? '\t' : ' ');
        spacing = (format.spacing ? " " : "");
        newline = (format.newline ? "\n" : spacing);
    }

    void Writer::WriteStream(const Node &node, std::ostream &stream) const
    {
        WriteNode(node, 0, stream);
    }
    FString Writer::WriteString(const Node &node) const
    {
        std::ostringstream outStream;
        WriteStream(node, outStream);
        return FString(outStream.str());
    }

    void Writer::WriteNode(const Node &node, uint32 level, std::ostream &stream) const
    {
        switch (node.GetType())
        {
            case Node::T_INVALID: break;
            case Node::T_OBJECT:
                WriteObject(node, level, stream); break;
            case Node::T_ARRAY:
                WriteArray(node, level, stream); break;
            case Node::T_NULL: // Fallthrough
            case Node::T_STRING: // Fallthrough
            case Node::T_NUMBER: // Fallthrough
            case Node::T_VALIDATION: // Fallthrough
            case Node::T_BOOL:
                WriteValue(node, stream); break;
        }
    }
    void Writer::WriteObject(const Node &node, uint32 level, std::ostream &stream) const
    {
        stream << "{" << newline;

        for (Node::const_iterator it = node.begin(); it != node.end(); ++it)
        {
            const FString &name = (*it).first;
            const Node &value = (*it).second;

            if (it != node.begin())
                stream << "," << newline;
            stream << GetIndentation(level + 1).GetAnsiCharString() << "\""<<name.GetAnsiCharString()<<"\"" << ":" << spacing;
            WriteNode(value, level + 1, stream);
        }

        stream << newline << GetIndentation(level).GetAnsiCharString() << "}";
    }
    void Writer::WriteArray(const Node &node, uint32 level, std::ostream &stream) const
    {
        stream << "[" << newline;

        for (Node::const_iterator it = node.begin(); it != node.end(); ++it)
        {
            const Node &value = (*it).second;

            if (it != node.begin())
                stream << "," << newline;
            stream << GetIndentation(level + 1).GetAnsiCharString();
            WriteNode(value, level + 1, stream);
        }

        stream << newline << GetIndentation(level).GetAnsiCharString() << "]";
    }
    void Writer::WriteValue(const Node &node, std::ostream &stream) const
    {
        if (node.IsString())
        {
            stream << "\""<< EscapeString(node.ToString()).GetAnsiCharString()<<"\"";
        }
        else
        {
            stream << node.ToString().GetAnsiCharString();
        }
    }

    FString Writer::GetIndentation(uint32 level) const
    {
        if (!format.newline)
        {
            return FString("");
        }
        else
        {
            return FString(format.indentSize * level, indentationChar);
        }
    }

    JsonParser::JsonParser() = default;
    JsonParser::~JsonParser() = default;

    Node JsonParser::ParseStream(std::istream &stream)
    {
        TokenQueue tokens;
        DataQueue data;

        Tokenize(stream, tokens, data);
        Node node = Assemble(tokens, data);

        return node;
    }

    const FString &JsonParser::GetError() const
    {
        return error;
    }

    void JsonParser::Tokenize(std::istream &stream, TokenQueue &tokens, DataQueue &data)
    {
        Token token = T_UNKNOWN;
        FString valueBuffer;
        bool saveBuffer;

        ANSICHAR c = '\0';
        while (stream.peek() != std::char_traits<ANSICHAR>::eof())
        {
            stream.get(c);

            if (isWhitespace(c))
                continue;

            saveBuffer = true;

            switch (c)
            {
                case '{':
                {
                    token = T_OBJ_BEGIN;
                    break;
                }
                case '}':
                {
                    token = T_OBJ_END;
                    break;
                }
                case '[':
                {
                    token = T_ARRAY_BEGIN;
                    break;
                }
                case ']':
                {
                    token = T_ARRAY_END;
                    break;
                }
                case ',':
                {
                    token = T_SEPARATOR_NODE;
                    break;
                }
                case ':':
                {
                    token = T_SEPARATOR_NAME;
                    break;
                }
                case '"':
                {
                    token = T_VALUE;
                    ReadString(stream, data);
                    break;
                }
                case '/':
                {
                    auto p = static_cast<ANSICHAR>(stream.peek());
                    if (p == '*')
                    {
                        JumpToCommentEnd(stream);
                        saveBuffer = false;
                        break;
                    }
                    else if (p == '/')
                    {
                        JumpToNext('\n', stream);
                        saveBuffer = false;
                        break;
                    }
                    // Intentional fallthrough
                }
                default:
                {
                    valueBuffer += c;
                    saveBuffer = false;
                    break;
                }
            }

            if ((saveBuffer || stream.peek() == std::char_traits<ANSICHAR>::eof()) && (!valueBuffer.IsEmpty())) // Always save buffer on the last character
            {
                if (InterpretValue(valueBuffer, data))
                {
                    tokens.push(T_VALUE);
                }
                else
                {
                    // Store the unknown token, so we can show it to the user
                    data.push(std::make_pair(Node::T_STRING, valueBuffer));
                    tokens.push(T_UNKNOWN);
                }

                valueBuffer.Empty();
            }

            // Push the token last so that any data
            // will get pushed first from above.
            // If saveBuffer is false, it means that
            // we are in the middle of a value, so we
            // don't want to push any tokens now.
            if (saveBuffer)
            {
                tokens.push(token);
            }
        }
    }
    Node JsonParser::Assemble(TokenQueue &tokens, DataQueue &data)
    {
        std::stack<NamedNode> nodeStack;
        Node root(Node::T_INVALID);

        FString nextName;

        Token token;
        while (!tokens.empty())
        {
            token = tokens.front();
            tokens.pop();

            switch (token)
            {
                case T_UNKNOWN:
                {
                    const FString &unknownToken = data.front().second;
                    error = FString("Unknown token: ") + unknownToken;
                    data.pop();
                    return Node(Node::T_INVALID);
                }
                case T_OBJ_BEGIN:
                {
                    nodeStack.push(std::make_pair(nextName, Node(Node::T_OBJECT)));
                    nextName.Empty();
                    break;
                }
                case T_ARRAY_BEGIN:
                {
                    nodeStack.push(std::make_pair(nextName, Node(Node::T_ARRAY)));
                    nextName.Empty();
                    break;
                }
                case T_OBJ_END:
                case T_ARRAY_END:
                {
                    if (nodeStack.empty())
                    {
                        error = "Found end of object or array without beginning";
                        return Node(Node::T_INVALID);
                    }
                    if (token == T_OBJ_END && !nodeStack.top().second.IsObject())
                    {
                        error = "Mismatched end and beginning of object";
                        return Node(Node::T_INVALID);
                    }
                    if (token == T_ARRAY_END && !nodeStack.top().second.IsArray())
                    {
                        error = "Mismatched end and beginning of array";
                        return Node(Node::T_INVALID);
                    }

                    FString nodeName = nodeStack.top().first;
                    Node node = nodeStack.top().second;
                    nodeStack.pop();

                    if (!nodeStack.empty())
                    {
                        Node &stackTop = nodeStack.top().second;
                        if (stackTop.IsObject())
                        {
                            stackTop.Add(nodeName, node);
                        }
                        else if (stackTop.IsArray())
                        {
                            stackTop.Add(node);
                        }
                        else
                        {
                            error = "Can only add elements to objects and arrays";
                            return Node(Node::T_INVALID);
                        }
                    }
                    else
                    {
                        root = node;
                    }
                    break;
                }
                case T_VALUE:
                {
                    if (data.empty())
                    {
                        error = "Missing data for value";
                        return Node(Node::T_INVALID);
                    }

                    const std::pair<Node::Type, FString> &dataPair = data.front();
                    if (!tokens.empty() && tokens.front() == T_SEPARATOR_NAME)
                    {
                        tokens.pop();
                        if (dataPair.first != Node::T_STRING)
                        {
                            error = "A name has to be a string";
                            return Node(Node::T_INVALID);
                        }
                        else
                        {
                            nextName = dataPair.second;
                            data.pop();
                        }
                    }
                    else
                    {
                        Node node(dataPair.first, dataPair.second);
                        data.pop();

                        if (!nodeStack.empty())
                        {
                            Node &stackTop = nodeStack.top().second;
                            if (stackTop.IsObject())
                                stackTop.Add(nextName, node);
                            else if (stackTop.IsArray())
                                stackTop.Add(node);

                            nextName.Empty();
                        }
                        else
                        {
                            error = "Outermost node must be an object or array";
                            return Node(Node::T_INVALID);
                        }
                    }
                    break;
                }
                case T_SEPARATOR_NAME:
                    break;
                case T_SEPARATOR_NODE:
                {
                    if (!tokens.empty() && tokens.front() == T_ARRAY_END) {
                        error = "Extra comma in array";
                        return Node(Node::T_INVALID);
                    }
                    break;
                }
            }
        }

        return root;
    }

    void JsonParser::JumpToNext(ANSICHAR c, std::istream &stream)
    {
        while (!stream.eof() && static_cast<ANSICHAR>(stream.get()) != c);
        stream.unget();
    }
    void JsonParser::JumpToCommentEnd(std::istream &stream)
    {
        stream.ignore(1);
        ANSICHAR c1 = '\0', c2 = '\0';
        while (stream.peek() != std::char_traits<ANSICHAR>::eof())
        {
            stream.get(c2);

            if (c1 == '*' && c2 == '/')
                break;

            c1 = c2;
        }
    }

    void JsonParser::ReadString(std::istream &stream, DataQueue &data)
    {
        FString str;

        ANSICHAR c1 = '\0', c2 = '\0';
        while (stream.peek() != std::char_traits<ANSICHAR>::eof())
        {
            stream.get(c2);

            if (c1 != '\\' && c2 == '"')
            {
                break;
            }

            str += c2;

            c1 = c2;
        }

        data.push(std::make_pair(Node::T_STRING, str));
    }
    bool JsonParser::InterpretValue(const FString &value, DataQueue &data)
    {
        FString upperValue = value.ToUpper();

        if (upperValue == FString("nullptr"))
        {
            data.push(std::make_pair(Node::T_NULL, FString("")));
        }
        else if (upperValue == FString("TRUE"))
        {
            data.push(std::make_pair(Node::T_BOOL, FString("true")));
        }
        else if (upperValue == FString("FALSE"))
        {
            data.push(std::make_pair(Node::T_BOOL, FString("false")));
        }
        else
        {
            bool number = true;
            bool negative = false;
            bool fraction = false;
            bool scientific = false;
            bool scientificSign = false;
            bool scientificNumber = false;
            for (int32 b = 0; b < upperValue.Len(); b++)
            {
                ANSICHAR c = upperValue.AtAnsi(static_cast<uint32>(b));
                switch (c)
                {
                    case '-':
                    {
                        if (scientific)
                        {
                            if (scientificSign) // Only one - allowed after E
                                number = false;
                            else
                                scientificSign = true;
                        }
                        else
                        {
                            if (negative) // Only one - allowed before E
                                number = false;
                            else
                                negative = true;
                        }
                        break;
                    }
                    case '+':
                    {
                        if (!scientific || scientificSign)
                            number = false;
                        else
                            scientificSign = true;
                        break;
                    }
                    case '.':
                    {
                        if (fraction) // Only one . allowed
                            number = false;
                        else
                            fraction = true;
                        break;
                    }
                    case 'E':
                    {
                        if (scientific)
                            number = false;
                        else
                            scientific = true;
                        break;
                    }
                    default:
                    {
                        if (c >= '0' && c <= '9')
                        {
                            if (scientific)
                                scientificNumber = true;
                        }
                        else
                        {
                            number = false;
                        }
                        break;
                    }
                }
            }

            if (scientific && !scientificNumber)
                number = false;

            if (number)
            {
                data.push(std::make_pair(Node::T_NUMBER, value));
            }
            else
            {
                return false;
            }
        }
        return true;
    }
}