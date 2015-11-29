#pragma once
#include <gumbo.h>
#include <string>
#include <algorithm>
#include <macro.h>

namespace AutoLogin
{
	namespace CrossPlatform
	{
		class MosMetroResponseParser final
		{
		public:
			static std::wstring GetPostString(const char* source) NOEXCEPT
			{
				using namespace std;

				wstring result;

				if (nullptr == source) return result;

				char authFormAttribute[] = "auth-form";
				const auto authFormLen = extent<decltype(authFormAttribute)>::value - 1;
				const auto sourceLen = strlen(source);

				auto authFormPointer = find_end(source, source + sourceLen,
												authFormAttribute, authFormAttribute + authFormLen);

				if (source + sourceLen == authFormPointer) return result;

				char formBeginTag[] = "<form";
				const auto formBeginTagLen = extent<decltype(formBeginTag)>::value - 1;

				char formEndTag[] = "</form>";
				const auto formEndTagLen = extent<decltype(formEndTag)>::value - 1;

				auto formBeginTagPointer = find_end(source, authFormPointer,
													formBeginTag, formBeginTag + formBeginTagLen);

				auto formEndTagPointer = search(authFormPointer, source + sourceLen,
												formEndTag, formEndTag + formEndTagLen);

				auto output = gumbo_parse(string(formBeginTagPointer, formEndTagPointer + formEndTagLen).data());

				if (!output) return result;

				auto formNode = FindNode(output->root,
										 GUMBO_TAG_FORM);

				if (!formNode) return result;

				result = GetFormParams(formNode);

				gumbo_destroy_output(&kGumboDefaultOptions,
									 output);

				return result;
			}

			static std::wstring GetAuthUrl(const char* source) NOEXCEPT
			{
				using namespace std;

				wstring result;

				if (nullptr == source) return result;

				auto output = gumbo_parse(source);

				if (!output) return result;

				auto headNode = FindNode(output->root,
										 GUMBO_TAG_HEAD);

				result = GetUrl(headNode);

				gumbo_destroy_output(&kGumboDefaultOptions,
									 output);

				return result;
			}

		private:
			static const GumboNode* FindNode(const GumboNode* node,
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

			static const GumboNode* FindNode(const GumboNode* node,
											 GumboTag tag,
											 const char* attributeId,
											 const char* attributeValue) NOEXCEPT
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

			static std::wstring GetFormParams(const GumboNode* formNode) NOEXCEPT
			{
				using namespace std;

				string result;

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
							if (_stricmp(attribute->name, "name") == 0)
							{
								name.append(attribute->value);
							}
							else if (_stricmp(attribute->name, "value") == 0)
							{
								value.append(attribute->value);
							}
							//���� � �������� �������, ����� ��������� ����
							if (!name.empty() && !value.empty())
							{
								break;
							}
						}

						result.append(name)
							  .append("=")
							  .append(value)
							  .append("&");
					}
				}

				if (!result.empty())
				{
					result = result.substr(0, result.size() - 1);
				}

				return wstring(begin(result), end(result));
			}

			static std::wstring GetUrl(const GumboNode* headNode) NOEXCEPT
			{
				using namespace std;

				string result;

				auto children = &headNode->v.element.children;
				for (unsigned i = 0; i < children->length; ++i)
				{
					auto child = static_cast<GumboNode*>(children->data[i]);
					if (GUMBO_TAG_META == child->v.element.tag)
					{
						auto attributes = &child->v.element.attributes;
						if (attributes->length <= 0) break;

						auto httpEquivAttribute = static_cast<GumboAttribute*>(attributes->data[0]);
						if (_stricmp(httpEquivAttribute->name, "http-equiv") != 0 || _stricmp(httpEquivAttribute->value, "refresh") != 0) continue;

						for (unsigned j = 1; j < attributes->length; ++j)
						{
							auto attribute = static_cast<GumboAttribute*>(attributes->data[j]);
							if (_stricmp(attribute->name, "content") == 0)
							{
								const char term[] = "http";
								auto first = strstr(attribute->value, term);
								if (nullptr != first)
								{
									result.append(first);
								}
							}
						}
					}
				}

				return wstring(begin(result), end(result));
			}
		};
	}
}
