#pragma once
#include <gumbo.h>
#include <algorithm>
#include <macro.h>
#include <PostData.h>

namespace AutoLogin
{
    namespace CrossPlatform
    {
        class MosMetroResponseParser final
        {
        public:
            static PostData GetPostData(const char *source,
                                        uint_fast32_t sourceLen) NOEXCEPT
            {
                using namespace std;

                PostData result;

                if (nullptr == source)
                {
                    return result;
                }

                char formBeginTag[] = "<form";
                const auto formBeginTagLen = extent<decltype(formBeginTag)>::value - 1;

                char formEndTag[] = "</form>";
                const auto formEndTagLen = extent<decltype(formEndTag)>::value - 1;

                auto formBeginTagPointer = find_end(source,
                                                    source + sourceLen,
                                                    formBeginTag,
                                                    formBeginTag + formBeginTagLen);

                if (formBeginTagPointer == source + sourceLen)
                {
                    return result;
                }

                auto formEndTagPointer = search(formBeginTagPointer,
                                                source + sourceLen,
                                                formEndTag,
                                                formEndTag + formEndTagLen);

                auto temp = string(formBeginTagPointer, formEndTagPointer + formEndTagLen);

                auto output = gumbo_parse(temp.data());

                if (output)
                {
                    auto formNode = FindNode(output->root,
                                             GUMBO_TAG_FORM);

                    if (formNode)
                    {
                        result = PostData(GetAction(formNode), GetFormParams(formNode));
                    }
                }

                gumbo_destroy_output(&kGumboDefaultOptions,
                                     output);

                return result;
            }

            static std::wstring GetAuthUrl(const char *source,
                                           uint_fast32_t sourceLen) NOEXCEPT
            {
                using namespace std;

                wstring result;

                if (nullptr == source)
                {
                    return result;
                }

                char headBeginTag[] = "<head";
                const auto headBeginTagLen = extent<decltype(headBeginTag)>::value - 1;

                char headEndTag[] = "</head>";
                const auto headEndTagLen = extent<decltype(headEndTag)>::value - 1;

                auto formBeginTagPointer = find_end(source,
                                                    source + sourceLen,
                                                    headBeginTag,
                                                    headBeginTag + headBeginTagLen);

                if (formBeginTagPointer == source + sourceLen)
                {
                    return result;
                }

                auto formEndTagPointer = search(formBeginTagPointer,
                                                source + sourceLen,
                                                headEndTag,
                                                headEndTag + headEndTagLen);

                auto temp = string(formBeginTagPointer, formEndTagPointer + headEndTagLen);

                auto output = gumbo_parse(temp.data());

                if (output)
                {
                    auto headNode = FindNode(output->root,
                                             GUMBO_TAG_HEAD);

                    if (headNode)
                    {
                        result = GetUrl(headNode);
                    }
                }

                gumbo_destroy_output(&kGumboDefaultOptions,
                                     output);

                return result;
            }

        private:
            static const GumboNode* FindNode(const GumboNode *node,
                                             GumboTag tag) NOEXCEPT
            {
                if (node->v.element.tag == tag)
                {
                    return node;
                }

                auto children = &node->v.element.children;
                for (unsigned i = 0; i < children->length; ++i)
                {
                    auto child = static_cast<GumboNode*>(children->data[i]);
                    if (GUMBO_NODE_ELEMENT == child->type)
                    {
                        auto result = FindNode(child, tag);
                        if (nullptr != result)
                        {
                            return result;
                        }
                    }
                }

                return nullptr;
            }

            static const GumboNode* FindNode(const GumboNode *node,
                                             GumboTag tag,
                                             const char *attributeId,
                                             const char *attributeValue) NOEXCEPT
            {
                if (node->v.element.tag == tag)
                {
                    auto attributes = &node->v.element.attributes;
                    for (unsigned j = 0; j < attributes->length; ++j)
                    {
                        auto attribute = static_cast<GumboAttribute*>(attributes->data[j]);
                        if (_stricmp(attribute->name, attributeId) == 0 && _stricmp(attribute->value, attributeValue) == 0)
                        {
                            return node;
                        }
                    }
                }

                auto children = &node->v.element.children;
                for (unsigned i = 0; i < children->length; ++i)
                {
                    auto child = static_cast<GumboNode*>(children->data[i]);
                    if (GUMBO_NODE_ELEMENT == child->type)
                    {
                        auto result = FindNode(child,
                                               tag,
                                               attributeId,
                                               attributeValue);
                        if (nullptr != result)
                        {
                            return result;
                        }
                    }
                }

                return nullptr;
            }

            static std::wstring GetAction(const GumboNode *formNode) NOEXCEPT
            {
                using namespace std;

                string temp;

                auto attributes = &formNode->v.element.attributes;
                for (unsigned j = 0; j < attributes->length; ++j)
                {
                    auto attribute = static_cast<GumboAttribute*>(attributes->data[j]);
                    if (strcmp(attribute->name, "action") == 0)
                    {
                        temp.append(attribute->value);
                        break;
                    }
                }

                return wstring(begin(temp), end(temp));
            }

            static std::wstring GetFormParams(const GumboNode *formNode) NOEXCEPT
            {
                using namespace std;

                string temp;

                auto children = &formNode->v.element.children;
                for (unsigned i = 0; i < children->length; ++i)
                {
                    auto child = static_cast<GumboNode*>(children->data[i]);
                    if (GUMBO_TAG_INPUT == child->v.element.tag)
                    {
                        string name, value;
                        auto attributes = &child->v.element.attributes;
                        for (unsigned j = 0; j < attributes->length; ++j)
                        {
                            auto attribute = static_cast<GumboAttribute*>(attributes->data[j]);
                            if (strcmp(attribute->name, "name") == 0)
                            {
                                name.append(attribute->value);
                            }
                            else if (strcmp(attribute->name, "value") == 0)
                            {
                                value.append(attribute->value);
                            }
                            //Ключ и значение найдены, можно прерывать цикл
                            if (!name.empty() && !value.empty())
                            {
                                break;
                            }
                        }

                        temp.append(name)
                            .append("=")
                            .append(value)
                            .append("&");
                    }
                }

                if (!temp.empty())
                {
                    temp = temp.substr(0, temp.size() - 1);
                }

                return wstring(begin(temp), end(temp));
            }

            static std::wstring GetUrl(const GumboNode *headNode) NOEXCEPT
            {
                using namespace std;

                auto children = &headNode->v.element.children;
                for (unsigned i = 0; i < children->length; ++i)
                {
                    auto child = static_cast<GumboNode*>(children->data[i]);
                    if (GUMBO_TAG_META == child->v.element.tag)
                    {
                        auto attributes = &child->v.element.attributes;
                        if (attributes->length <= 0)
                        {
                            continue;
                        }

                        auto httpEquivAttribute = static_cast<GumboAttribute*>(attributes->data[0]);
                        if (_stricmp(httpEquivAttribute->name, "http-equiv") != 0 || _stricmp(httpEquivAttribute->value, "refresh") != 0)
                        {
                            continue;
                        }

                        for (unsigned j = 1; j < attributes->length; ++j)
                        {
                            auto attribute = static_cast<GumboAttribute*>(attributes->data[j]);
                            if (_stricmp(attribute->name, "content") == 0)
                            {
                                const char term[] = "http";
                                auto first = strstr(attribute->value, term);
                                string temp;
                                if (nullptr != first)
                                {
                                    temp.append(first);
                                }
                                return wstring(begin(temp), end(temp));
                            }
                        }
                    }
                }

                return wstring();
            }
        };
    }
}
