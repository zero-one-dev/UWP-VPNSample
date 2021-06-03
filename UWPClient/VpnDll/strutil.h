#pragma once

void rtrimwsv(std::wstring_view& sv)
{
	size_t end = sv.find_last_not_of(L' ');
	if (end != std::wstring_view::npos)
		sv.remove_suffix(sv.size() - end - 1);
}

// https://www.bfilipek.com/2018/07/string-view-perf-followup.html
std::vector<std::wstring_view> splitwsv(std::wstring_view strv, wchar_t delim = L' ')
{
	std::vector<std::wstring_view> output;
	size_t first = 0;

	while (first < strv.size())
	{
		const auto second = strv.find_first_of(delim, first);

		if (first != second)
			output.emplace_back(strv.substr(first, second - first));

		if (second == std::string_view::npos)
			break;

		first = second + 1;
	}

	return output;
}

std::vector<std::wstring> splitws(std::wstring_view strv, wchar_t delim = L' ')
{
	std::vector<std::wstring> output;
	size_t first = 0;

	while (first < strv.size())
	{
		const auto second = strv.find_first_of(delim, first);

		if (first != second)
			output.emplace_back(strv.substr(first, second - first));

		if (second == std::string_view::npos)
			break;

		first = second + 1;
	}

	return output;
}

void split_string(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
    std::string::size_type pos1, pos2;
    size_t len = s.length();
    pos2 = s.find(c);
    pos1 = 0;
    while (std::string::npos != pos2)
    {
        v.emplace_back(s.substr(pos1, pos2 - pos1));

        pos1 = pos2 + c.size();
        pos2 = s.find(c, pos1);
    }
    if (pos1 != len)
    {
        v.emplace_back(s.substr(pos1));
    }
}

void split_string(const std::wstring& s, std::vector<std::wstring>& v, const std::wstring& c)
{
    std::wstring::size_type pos1, pos2;
    size_t len = s.length();
    pos2 = s.find(c);
    pos1 = 0;
    while (std::wstring::npos != pos2)
    {
        v.emplace_back(s.substr(pos1, pos2 - pos1));

        pos1 = pos2 + c.size();
        pos2 = s.find(c, pos1);
    }
    if (pos1 != len)
    {
        v.emplace_back(s.substr(pos1));
    }
}

std::map<std::string, std::string> parse_param(std::string str)
{
    std::map<std::string, std::string> m;
    std::vector<std::string> v;
    split_string(str, v, "&");
    for (auto&& param_item : v)
    {
        std::vector<std::string> v2;
        split_string(param_item, v2, "=");
        if (v2.size() == 2)
        {
            m[v2[0]] = v2[1];
        }
    }
    return m;
}

std::map<std::wstring, std::wstring> parse_param(std::wstring str)
{
    std::map<std::wstring, std::wstring> m;
    std::vector<std::wstring> v;
    split_string(str, v, L"&");
    for (auto&& param_item : v)
    {
        std::vector<std::wstring> v2;
        split_string(param_item, v2, L"=");
        if (v2.size() == 2)
        {
            m[v2[0]] = v2[1];
        }
    }
    return m;
}

std::string make_param(std::map<std::string, std::string> param)
{
    std::string str = "";
    bool is_first_item = true;
    for (auto&& param_item : param)
    {
        std::string key = param_item.first;
        std::string value = param_item.second;
        if (is_first_item == false)
        {
            str += "&";
        }
        is_first_item = false;
        str += (key + "=" + value);
    }
    return str;
}

std::wstring make_param(std::map<std::wstring, std::wstring> param)
{
    std::wstring str = L"";
    bool is_first_item = true;
    for (auto&& param_item : param)
    {
        std::wstring key = param_item.first;
        std::wstring value = param_item.second;
        if (is_first_item == false)
        {
            str += L"&";
        }
        is_first_item = false;
        str += (key + L"=" + value);
    }
    return str;
}

