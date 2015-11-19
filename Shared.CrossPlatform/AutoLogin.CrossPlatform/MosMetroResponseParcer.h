#pragma once
#include <string>
#include <gumbo.h>
#include <macro.h>

namespace AutoLogin
{
	namespace CrossPlatform
	{
		class MosMetroResponseParser final
		{
		public:
			static std::wstring GetPostString(std::string source) NOEXCEPT
			{
				using namespace std;

				wstring result;

				if (source.empty()) return result;

				auto output = gumbo_parse(source.data());

				if (!output) return result;

				auto formNode = FindNodeByTag(output->root,
											  GUMBO_TAG_FORM);

				if (!formNode) return result;

				result = GetFormParams(formNode);

				gumbo_destroy_output(&kGumboDefaultOptions,
									 output);

				return result;
			}

			static std::wstring GetFormUrl(std::string source) NOEXCEPT
			{
				using namespace std;

				wstring result;

				if (source.empty()) return result;

				auto output = gumbo_parse(source.data());

				if (!output) return result;

				auto headNode = FindNodeByTag(output->root,
											  GUMBO_TAG_HEAD);

				result = GetUrl(headNode);

				gumbo_destroy_output(&kGumboDefaultOptions,
									 output);

				return result;
			}

		private:
			static const GumboNode* FindNodeByTag(const GumboNode* node,
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
						auto result = FindNodeByTag(child, tag);
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
							//Ключ и значение найдены, можно прерывать цикл
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
