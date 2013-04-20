#include <stdio.h>
 
#include <string>
#include <map>
#include <set>
#include <iostream>
 
using namespace std;

#define INT_BUFFER 15

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

typedef map<CIndex, CCell*> CSheet;
 
class CIndex 
{
public:
	int x, y;
	CIndex(int x, int y): x(x), y(y) {}
	bool operator<(const CIndex& idx) const
	{
		return (x < idx.x) || (x == idx.x && y < idx.y);
	}
	bool operator==(const CIndex& idx) const
	{
		return (x == idx.x) && (y == idx.y);
	}
	bool operator!=(const CIndex& idx) const
	{
		return (x != idx.x) || (y != idx.y);
	}
	bool operator!() const
	{
		return (x < 0 || y < 0);
	}
	operator bool() const
	{
		return (x >= 0 && y >= 0);
	}
	CIndex(const string& str) 
	{
		x = y = 0;
		bool flag = false;
		const char *ptr = str.c_str();
		while(*ptr)
		{
			if (isalpha(*ptr))
			{
				x = x*('Z'-'A'+1) + (toupper(*ptr) - 'A');
				flag = true;
			} else
				break;
			++ptr;
		}
		if (!flag || !*ptr || !isnumber(ptr))
			x = y = -1;
		else
			y = atoi(ptr);
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
	CCell(CSheet& sheet, const CIndex& tag, const string& text): 
		sheet(sheet), tag(tag), text(text), bCalculated(false)
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
	virtual CCell* clone() const
	{
		return new CCell(*this);
	}
};

class CCellError: public CCell
{
public:
	CCellError(const CCell& cell, const string& msg): CCell(cell)
	{
		disptext = msg;
		bCalculated = true;
	}
	CCell* clone() const
	{
		return new CCellError(*this);
	}
};

class CCellEmpty: public CCell
{
public:
	CCellEmpty(CSheet& sheet, const CIndex& tag): CCell(sheet, tag, string())
	{
	}
	CCellEmpty(const CCell& cell): CCell(cell)
	{
		if (text.length() != 0)
		{
			replace_with(new CCellError(*this, "#NOTEMPTY"));
			return;
		}
		bCalculated = true;
	}
	CCell* clone() const
	{
		return new CCellEmpty(*this);
	}
};

class CCellString: public CCell
{
public:
	CCellString(const CCell& cell): CCell(cell)
	{
		if (text.length() < 1 || text[0] != '\'')
		{
			replace_with(new CCellError(*this, "#INTERNAL"));
			return;
		}
		disptext = text.substr(1);
		bCalculated = true;
	}
	CCell* clone() const
	{
		return new CCellString(*this);
	}
};

class CCellNumber: public CCell
{
public:
	int value;
	CCellNumber(const CCell& cell, int value): CCell(cell), value(value)
	{
		char buffer[INT_BUFFER];
		_itoa(value, buffer, 10);
		disptext = buffer;
		bCalculated = true;
	}
	CCellNumber(const CCell& cell, const string& txt): CCell(cell)
	{
		disptext = txt;
		bCalculated = isnumber(txt);
		value = bCalculated ? atoi(txt.c_str()) : 0;
	}
	CCellNumber(const CCell& cell): CCell(cell)
	{
		if (!isnumber(text))
		{
			replace_with(new CCellError(*this, "#NOTANUMBER"));
			return;
		}
		disptext = text;
		value = atoi(text.c_str());
		bCalculated = true;
	}
	CCell* clone() const
	{
		return new CCellNumber(*this);
	}
};

class CCellFormula: public CCell
{
public:
	enum Operation {
		ENONE, EPLUS, EMINUS, EMULTIPLY, EDIVIDE
	};
	CCellFormula(const CCell& cell): CCell(cell)
	{
		if (text.length() < 1 || text[0] != '=')
		{
			replace_with(new CCellError(*this, "#NOTEQUATION"));
			return;
		}
	}
	bool calculate(set<CIndex>& refs);
	void calculate()
	{
		if (!bCalculated)
			calculate(set<CIndex>());
	}
	CCell* clone() const
	{
		return new CCellFormula(*this);
	}
};

class CCellUndefined: public CCell
{
public:
	CCellUndefined(CSheet& sheet, const CIndex& tag, const string& text): 
		CCell(sheet, tag, text)
	{
	}
	bool try_replace(CCell* cell)
	{
		cell = sheet[tag] ? sheet[tag] : cell;
		if (dynamic_cast<CCellError*>(cell) == NULL)
		{
			replace_with(cell);
			return true;
		} else {
			delete cell;
			sheet[tag] = NULL;
			return false;
		}
	}
	void calculate()
	{
		sheet[tag] = NULL;
		if (!(
			try_replace(new CCellEmpty(*this)) ||
			try_replace(new CCellString(*this)) ||
			try_replace(new CCellNumber(*this)) ||
			try_replace(new CCellFormula(*this))
			) )
		{
			replace_with(new CCellError(*this, "#NOPATTERN"));
			return;
		}
	}
	CCell* clone() const
	{
		return NULL;
	}
};

bool CCellFormula::calculate(set<CIndex>& refs)
{
	CIndex ltag = tag;
	if (refs.find(ltag) != refs.end())
		return false;
	refs.insert(ltag);

	string celltext = text.substr(1);
	int epos;
	int itemnum = 0;
	long long result = 0;
	int argument;
	Operation lastop = ENONE;

	while (celltext.length() != 0) {
		epos = celltext.find_first_of("+-*/");
		string item = celltext.substr(0, epos);
		if (epos == string::npos && lastop == ENONE) {
			if (isnumber(item)) {
				replace_with(new CCellNumber(*this, item));
				goto error;
			} else {
				CIndex idx(item);
				if (idx) {
					CSheet::iterator it = sheet.find(idx);
					CCell *cell = (it != sheet.end()) ? it->second : NULL;
					if (cell) {
						if (dynamic_cast<CCellFormula*>(cell))
						{
							if (!dynamic_cast<CCellFormula*>(cell)->calculate(refs)) {
								replace_with(new CCellError(*this, "#RECURSIVELINKS"));
								goto error2;
							}
							cell = sheet[idx];
						}
						cell = cell->clone();
						if (cell) {
							replace_with(cell);
							goto error;
						}
						replace_with(new CCellError(*this, "#INTERNAL"));
						goto error;
					}
				}
				replace_with(new CCellError(*this, "#INVALIDREFERENCE"));
				goto error;
			}
		}

		if (isnumber(item)) {
			argument = atoi(item.c_str());
		} else {
			CIndex idx(item);
			if (idx) {
				CSheet::iterator it = sheet.find(idx);
				CCell *cell = (it != sheet.end()) ? it->second : NULL;
				if (cell && dynamic_cast<CCellUndefined*>(cell) != NULL) {
					cell->calculate();
					cell = sheet[idx];
				}
				if (cell && dynamic_cast<CCellFormula*>(cell)) {
					if (!dynamic_cast<CCellFormula*>(cell)->calculate(refs)) {
						replace_with(new CCellError(*this, "#RECURSIVELINKS"));
						goto error2;
					}
					cell = sheet[idx];
				}
				if (cell) {
					if (dynamic_cast<CCellNumber*>(cell))
						argument = dynamic_cast<CCellNumber*>(cell)->value;
					else {
						replace_with(new CCellError(*this, "#INVALIDTYPE"));
						goto error;
					}
				} else {
					replace_with(new CCellError(*this, "#INVALIDREFERENCE"));
					goto error;
				}
			} else {
				replace_with(new CCellError(*this, "#INVALIDREFERENCE"));
				goto error;
			}
		}
		switch (lastop) {
			case EPLUS:
				result += argument;
				break;
			case EMINUS:
				result -= argument;
				break;
			case EMULTIPLY:
				result *= argument;
				break;
			case EDIVIDE:
				result /= argument;
				break;
			default:
				result = argument;
				break;
		}
		if (epos != string::npos) {
			switch (celltext[epos]) {
				case '+': lastop = EPLUS;     break;
				case '-': lastop = EMINUS;    break; 
				case '*': lastop = EMULTIPLY; break;
				case '/': lastop = EDIVIDE;   break;
				default:
					replace_with(new CCellError(*this, "#INVALIDOP"));
					goto error;
			}
		} else {
			lastop = ENONE;
			celltext = string();
			break;
		}
		celltext = celltext.substr(epos+1);
	}
	bCalculated = true;
	replace_with(new CCellNumber(*this, (int)result));
error:
	refs.erase(ltag);
	return true;
error2:
	refs.erase(ltag);
	return false;
}


int main(int argc, char* argv[])
{
	CSheet sheet;
	int n, m;
	string str;

	if (!getline(cin, str))
		return -1;
	int found = str.find_first_of("\t");
	if (found == string::npos)
		return -1;
	string nstr = str.substr(0, found);
	string mstr = str.substr(found + 1);
	if (isnumber(nstr) && isnumber(mstr))
	{
		n = atoi(nstr.c_str());
		m = atoi(mstr.c_str());
	} else {
		return -1;
	}

	for (int i = 0; i < n; ++i)
	{
		if (!getline(cin, str))
		{
			for (int j = 0; j < m; ++j)
			{
				CIndex idx(j, i+1);
				sheet[idx] = new CCellEmpty(sheet, idx);
			}
			continue;
		}
		string subtext;
		int bfound = 0;
		int efound = 0;
		for (int j = 0; j < m; ++j)
		{
			CIndex idx(j, i+1);
			if (efound == string::npos)
			{
				sheet[idx] = new CCellEmpty(sheet, idx);
				continue;
			}

			efound = str.find_first_of("\t", bfound);
			if (efound != string::npos)
				subtext = str.substr(bfound, efound - bfound);
			else
				subtext = str.substr(bfound);

			CCell *cell = new CCellUndefined(sheet, idx, subtext);
			sheet[idx] = cell;
			cell->calculate();

			if (efound != string::npos)
				bfound = efound + 1;
		}
	}

	for(CSheet::iterator it = sheet.begin(); it != sheet.end(); ++it) {
		it->second->calculate();
	}

	for (int i = 0; i < n; ++i)
	{
		for (int j = 0; j < m; ++j)
		{
			cout << sheet[CIndex(j,i+1)]->get_text() <<
				((j != m-1) ? '\t' : '\n');
		}
	}

#ifdef _CONSOLE
	// for Visual Studio only
	system("pause");
#endif

	return 0;
}
