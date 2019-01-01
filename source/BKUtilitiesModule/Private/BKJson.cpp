// Copyright Burak Kara, All rights reserved.

#include "BKJson.h"
#include <fstream>
#include <cassert>

namespace BKJson
{
    namespace
    {
        inline bool isWhitespace(UTFCHAR c)
        {
            return (c == L'\n' || c == L' ' || c == L'\t' || c == L'\r' || c == L'\f');
        }

        const UTFCHAR charsUnescaped[] = { L'\\'  , L'/'  , L'\"'  , L'\n' , L'\t' , L'\b' , L'\f' , L'\r' };
        const UTFCHAR *charsEscaped[]  = { L"\\\\", L"\\/", L"\\\"", L"\\n", L"\\t", L"\\b", L"\\f", L"\\r" };
        const uint32 numEscapeChars = 8;
        const UTFCHAR nullUnescaped = L'\0';
        const UTFCHAR *nullEscaped  = L"\0\0";
        const UTFCHAR *getEscaped(const UTFCHAR c)
        {
            for (uint32 i = 0; i < numEscapeChars; ++i)
            {
                const UTFCHAR &ue = charsUnescaped[i];

                if (c == ue)
                {
                    return charsEscaped[i];
                }
            }
            return nullEscaped;
        }
        UTFCHAR getUnescaped(const UTFCHAR c1, const UTFCHAR c2)
        {
            for (uint32 i = 0; i < numEscapeChars; ++i)
            {
                const UTFCHAR *e = charsEscaped[i];

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
    Node::Node(const UTFCHAR *value) : data(new Data(T_STRING)) { Set(value); }
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
                return FString(L"null");
            }
            else if (IsValidation())
            {
                return FString(L"validation");
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
            return (data->valueStr == FString(L"true"));
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
    void Node::Set(const UTFCHAR *value)
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
		FStringStream sstr;\
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
            data->valueStr = (value ? L"true" : L"false");
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
    Node &Node::operator=(const UTFCHAR *rhs) { Set(rhs); return *this; }
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
            data->children.emplace_back(FString(L""), node);
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

        UTFCHAR c;
        for (uint32 i = 0; i < value.Len(); i++)
        {
            c = (UTFCHAR)value.AtWide(i);

            const UTFCHAR *a = getEscaped(c);
            if (a[0] != L'\0')
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

        UTFCHAR c1, c2;
        for (uint32 i = 0; i < value.Len(); i++)
        {
            c1 = (UTFCHAR)value.AtWide(i);
            if ((i + 1) != value.Len())
            {
                c2 = (UTFCHAR)value.AtWide(i + 1);
                i++;
            }
            else
            {
                c2 = L'\0';
            }

            const UTFCHAR a = getUnescaped(c1, c2);
            if (a != L'\0')
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
        indentationChar = static_cast<UTFCHAR>(format.useTabs ? L'\t' : L' ');
        spacing = (format.spacing ? L" " : L"");
        newline = (format.newline ? L"\n" : spacing);
    }

    void Writer::WriteStream(const Node &node, FStringStream &stream) const
    {
        WriteNode(node, 0, stream);
    }
    FString Writer::WriteString(const Node &node) const
    {
        FStringStream outStream;
        WriteStream(node, outStream);
        return FString(outStream.Str());
    }

    void Writer::WriteNode(const Node &node, uint32 level, FStringStream &stream) const
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
    void Writer::WriteObject(const Node &node, uint32 level, FStringStream &stream) const
    {
        stream << L"{";
        stream << newline;

        for (Node::const_iterator it = node.begin(); it != node.end(); ++it)
        {
            const FString &name = (*it).first;
            const Node &value = (*it).second;

            if (it != node.begin())
            {
                stream << L",";
                stream << newline;
            }
            stream << GetIndentation(level + 1);
            stream << L"\"";
            stream << name;
            stream << L"\"";
            stream << L":";
            stream << spacing;
            WriteNode(value, level + 1, stream);
        }

        stream << newline;
        stream << GetIndentation(level);
        stream << L"}";
    }
    void Writer::WriteArray(const Node &node, uint32 level, FStringStream &stream) const
    {
        stream << L"[";
        stream << newline;

        for (Node::const_iterator it = node.begin(); it != node.end(); ++it)
        {
            const Node &value = (*it).second;

            if (it != node.begin())
            {
                stream << L",";
                stream << newline;
            }
            stream << GetIndentation(level + 1);
            WriteNode(value, level + 1, stream);
        }

        stream << newline;
        stream << GetIndentation(level);
        stream << L"]";
    }
    void Writer::WriteValue(const Node &node, FStringStream &stream) const
    {
        if (node.IsString())
        {
            stream << L"\"";
            stream << EscapeString(node.ToString());
            stream << L"\"";
        }
        else
        {
            stream << node.ToString();
        }
    }

    FString Writer::GetIndentation(uint32 level) const
    {
        if (!format.newline)
        {
            return FString(L"");
        }
        else
        {
            return FString(format.indentSize * level, indentationChar);
        }
    }

    JsonParser::JsonParser() = default;
    JsonParser::~JsonParser() = default;

    Node JsonParser::ParseStream(std::wistream &stream)
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

    void JsonParser::Tokenize(std::wistream &stream, TokenQueue &tokens, DataQueue &data)
    {
        Token token = T_UNKNOWN;
        FString valueBuffer;
        bool saveBuffer;

        UTFCHAR c = L'\0';
        while (stream.peek() != std::char_traits<UTFCHAR>::eof())
        {
            stream.get(c);

            if (isWhitespace(c))
                continue;

            saveBuffer = true;

            switch (c)
            {
                case L'{':
                {
                    token = T_OBJ_BEGIN;
                    break;
                }
                case L'}':
                {
                    token = T_OBJ_END;
                    break;
                }
                case L'[':
                {
                    token = T_ARRAY_BEGIN;
                    break;
                }
                case L']':
                {
                    token = T_ARRAY_END;
                    break;
                }
                case L',':
                {
                    token = T_SEPARATOR_NODE;
                    break;
                }
                case L':':
                {
                    token = T_SEPARATOR_NAME;
                    break;
                }
                case L'"':
                {
                    token = T_VALUE;
                    ReadString(stream, data);
                    break;
                }
                case L'/':
                {
                    auto p = static_cast<UTFCHAR>(stream.peek());
                    if (p == L'*')
                    {
                        JumpToCommentEnd(stream);
                        saveBuffer = false;
                        break;
                    }
                    else if (p == L'/')
                    {
                        JumpToNext(L'\n', stream);
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

            if ((saveBuffer || stream.peek() == std::char_traits<UTFCHAR>::eof()) && (!valueBuffer.IsEmpty())) // Always save buffer on the last character
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
                    error = FString(L"Unknown token: ") + unknownToken;
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
                        error = L"Found end of object or array without beginning";
                        return Node(Node::T_INVALID);
                    }
                    if (token == T_OBJ_END && !nodeStack.top().second.IsObject())
                    {
                        error = L"Mismatched end and beginning of object";
                        return Node(Node::T_INVALID);
                    }
                    if (token == T_ARRAY_END && !nodeStack.top().second.IsArray())
                    {
                        error = L"Mismatched end and beginning of array";
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
                            error = L"Can only add elements to objects and arrays";
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
                        error = L"Missing data for value";
                        return Node(Node::T_INVALID);
                    }

                    const std::pair<Node::Type, FString> &dataPair = data.front();
                    if (!tokens.empty() && tokens.front() == T_SEPARATOR_NAME)
                    {
                        tokens.pop();
                        if (dataPair.first != Node::T_STRING)
                        {
                            error = L"A name has to be a string";
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
                            error = L"Outermost node must be an object or array";
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
                        error = L"Extra comma in array";
                        return Node(Node::T_INVALID);
                    }
                    break;
                }
            }
        }

        return root;
    }

    void JsonParser::JumpToNext(UTFCHAR c, std::wistream &stream)
    {
        while (!stream.eof() && static_cast<UTFCHAR>(stream.get()) != c);
        stream.unget();
    }
    void JsonParser::JumpToCommentEnd(std::wistream &stream)
    {
        stream.ignore(1);
        UTFCHAR c1 = L'\0', c2 = L'\0';
        while (stream.peek() != std::char_traits<UTFCHAR>::eof())
        {
            stream.get(c2);

            if (c1 == L'*' && c2 == L'/')
                break;

            c1 = c2;
        }
    }

    void JsonParser::ReadString(std::wistream &stream, DataQueue &data)
    {
        FString str;

        UTFCHAR c1 = L'\0', c2 = L'\0';
        while (stream.peek() != std::char_traits<UTFCHAR>::eof())
        {
            stream.get(c2);

            if (c1 != L'\\' && c2 == L'"')
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

        if (upperValue == FString(L"nullptr"))
        {
            data.push(std::make_pair(Node::T_NULL, FString(L"")));
        }
        else if (upperValue == FString(L"TRUE"))
        {
            data.push(std::make_pair(Node::T_BOOL, FString(L"true")));
        }
        else if (upperValue == FString(L"FALSE"))
        {
            data.push(std::make_pair(Node::T_BOOL, FString(L"false")));
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
                UTFCHAR c = upperValue.AtWide(static_cast<uint32>(b));
                switch (c)
                {
                    case L'-':
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
                    case L'+':
                    {
                        if (!scientific || scientificSign)
                            number = false;
                        else
                            scientificSign = true;
                        break;
                    }
                    case L'.':
                    {
                        if (fraction) // Only one . allowed
                            number = false;
                        else
                            fraction = true;
                        break;
                    }
                    case L'E':
                    {
                        if (scientific)
                            number = false;
                        else
                            scientific = true;
                        break;
                    }
                    default:
                    {
                        if (c >= L'0' && c <= L'9')
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