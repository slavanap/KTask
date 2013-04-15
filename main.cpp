#include <stdio.h>
 
#include <string>
#include <map>
 
using namespace std;

bool isnumber(const string& str)
{
	if (str.length() == 0)
		return false;
	string::const_iterator it = str.begin();
	if ((*it == '+' || *it == '-') && str.length() > 1)
		++it;
	while(it != str.end())
	{
		if (!isdigit(*it))
			return false;
		++it;
	}
	return true;
}

class CIndex;
class CCell;

typedef map<CIndex, CCell> CSheet;
 
class CIndex 
{
public:
	int x, y;
	CIndex(int x, int y): x(x), y(y) {}
	bool operator<(const CIndex& idx)
	{
		return (x < idx.x) || (x == idx.x && y < idx.y);
	}
	bool operator==(const CIndex& idx)
	{
		return (x == idx.x) && (y == idx.y);
	}
	bool operator!=(const CIndex& idx)
	{
		return (x != idx.x) || (y != idx.y);
	}
};
 
class CCell 
{
protected:
	CSheet& sheet;
	CIndex tag;
	string text, disptext;
	bool bCalculated;
public:
	CCell(const CSheet& sheet, const CIndex& tag, const string& text): 
		sheet(sheet), tag(tag), text(text), bCalculated(false)
	{
	}
	CCell(CCell&& cell): tag(std::move(tag)), text(std::move(text))
	{
	}
	string get_text() const 
	{
		return disptext;
	}
	virtual void calculate()
	{
	}
	CCell* replace_with(CCell* cell)
	{
		sheet[tag] = cell;
		delete this;
		return cell;
	}
};

class CCellError: public CCell
{
public:
	CCellError(CCell&& cell, const string& msg): CCell(cell)
	{
		disptext = msg;
		bCalculated = true;
	}
};

class CCellEmpty: public CCell
{
public:
	CCellEmpty(CCell&& cell): CCell(cell)
	{
		if (text.length() != 0)
		{
			replace_with(new CCellError(std::move(*this), "#NOTEMPTY");
			return;
		}
		bCalculated = true;
	}
};

class CCellString: public CCell
{
public:
	CCellString(CCell&& cell): CCell(cell)
	{
		if (text.length() < 1 || text[0] != '\'')
		{
			replace_with(new CCellError(std::move(*this), "#INTERNAL");
			return;
		}
		disptext = text.substr(1);
		bCalculated = true;
	}
};

class CCellNumber: public CCell
{
public:
	int value;
	CCellNumber(CCell&& cell): CCell(cell)
	{
		if (!isnumber(text))
		{
			replace_with(new CCellError(std::move(*this), "#NOTANUMBER");
			return;
		}
		disptext = text;
		value = atoi(text.c_str());
		bCalculated = true;
	}
};

class CCellFormula: public CCell
{
public:
	CCellFormula(CCell&& cell): CCell(cell)
	{
		if (text.length() < 1 || text[0] != '=')
		{
			replace_with(new CCellError(std::move(*this), "#NOTEQUATION");
			return;
		}
	}
	void calculate(set<CIndex>& refs)
	{
		if (refs.find(CIndex) != refs.end())
		{
			replace_with(new CCellError(std::move(*this), "RECURSIVELINKS"));
			return;
		}
		refs.insert(tag);
		
		// calculate the equation
		// ...
		//
		
		bCalculated = true;		
	}
	void calculate()
	{
		if (!bCalculated)
			calculate(set<CIndex>());
	}
};

class CCellUndefined: public CCell
{
	bool try_replace(CCell* cell)
	{
		if (dynamic_cast<CCellError*>(sheet[tag]) == NULL)
		{
			delete this;
			return true;
		} else {
			delete cell;
			return false;
		}
	}
	void calculate()
	{
		if (!(
			try_replace(new CCellEmpty(*this)) ||
			try_replace(new CCellString(*this)) ||
			try_replace(new CCellNumber(*this)) ||
			try_replace(new CCellFormula(*this))
			) )
		{
			replace_with(new CCellError(std::move(*this), "#NOPATTERN");
			return;
		}
	}
};

int main(int argc, char* argv[])
{
	CSheet sheet;

	return 0;
}
