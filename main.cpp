#include <stdio.h>
#include <cstdlib>
#include <iostream>

#include <string>
#include <map>
#include <set>

using namespace std;

bool isnumber(const string& str)
{
	if (str.length() == 0)
		return false;
	string::const_iterator it = str.begin();
	//if ((*it == '+' || *it == '-') && str.length() > 1)
	//	++it;
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
	CIndex(string& str)
	{
        if (str[0] >= 'A' && str[0] <= 'Z')
        {
            x = str[0] - 'A';
        } else {
            x = str[0] - 'a';
        }
        y = atoi(str.substr(1).c_str());
	}
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
		sheet(sheet), tag(tag), text(text), bCalculated(false) {}
	CCell(CCell&& cell): sheet(sheet), tag(move(tag)), text(move(text)) {}
	string get_text() const
	{
		return disptext;
	}
	virtual void calculate() {}
	virtual void calculate(set<CIndex>& refs) {}
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
	CCellError(CCell&& cell, const string& msg): CCell(move(cell))
	{
		disptext = msg;
		bCalculated = true;
	}
};

class CCellEmpty: public CCell
{
public:
	CCellEmpty(CCell&& cell): CCell(move(cell))
	{
		if (text.length() != 0)
		{
			replace_with(new CCellError(move(*this), "#NOTEMPTY"));
			return;
		}
		bCalculated = true;
	}
};

class CCellString: public CCell
{
public:
	CCellString(CCell&& cell): CCell(move(cell))
	{
		if (text.length() < 1 || text[0] != '\'')
		{
			replace_with(new CCellError(move(*this), "#INTERNAL"));
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
	CCellNumber(CCell&& cell): CCell(move(cell))
	{
		if (!isnumber(text))
		{
			replace_with(new CCellError(move(*this), "#NOTANUMBER"));
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
	CCellFormula(CCell&& cell, set<CIndex>& refs): CCell(move(cell))
	{
		if (text.length() < 1 || text[0] != '=')
		{
			replace_with(new CCellError(move(*this), "#NOTEQUATION"));
			return;
		}
		calculate(refs);
		bCalculated = true;
	}
	void calculate(set<CIndex>& refs)
	{
		if (refs.find(tag) != refs.end())
		{
			replace_with(new CCellError(move(*this), "#RECURSIVELINKS"));
			return;
		}
		refs.insert(tag);

		// calculate the equation
		// ...
		//
        string subtext = text.substr(1);
        int fpos;
        if ((fpos = subtext.find_first_of("+-*/")) == string::npos)
        {
            if (isalpha(subtext[0]))
            {
                if (isnumber(subtext.substr(1)))
                {
                    sheet[CIndex(x,y)] -> calculate(refs);
                    replace_with(sheet[CIndex(x,y)]);
                } else {
                    replace_with(new CCellError(move(*this), "#BADLINK"));
                }
                return;
            } else if (isnumber(subtext)) {
                // create CCellNumber and replace
                // ...
                //
            }
        } else {

        }

		bCalculated = true;
	}
};

class CCellUndefined: public CCell
{
public:
    CCellUndefined(CCell&& cell): CCell(move(cell))
	{
	}
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
	void calculate(set<CIndex>& refs)
	{
		if (!(
			try_replace(new CCellEmpty(move(*this))) ||
			try_replace(new CCellString(move(*this))) ||
			try_replace(new CCellNumber(move(*this))) ||
			try_replace(new CCellFormula(move(*this), refs))
			) )
		{
			replace_with(new CCellError(move(*this), "#NOPATTERN"));
			return;
		}
	}
	void calculate()
	{
	    set<CIndex> emptySet;
	    calculate(emptySet);
	}
};

int main(int argc, char* argv[])
{
	CSheet sheet;

    int n,m;
    string str;

    //cin >> n >> m;
    //cin.get(); // for '\n' in first line

    // read n and m
    if (!getline(cin, str))
        return 0;
    int found = str.find_first_of("\t");
    if (found == string::npos)
        return 0;

    string nstr = str.substr(0, found);
    string mstr = str.substr(found + 1);
    if (isnumber(nstr) && isnumber(mstr))
    {
        n = atoi(nstr.c_str());
        m = atoi(mstr.c_str());
    } else {
        return 0;
    }

    for (int i = 0; i < n; ++i)
    {
        if (!getline(cin, str))
        {
            for (int j = 0; j < m; ++j)
            {
                CIndex ind(i,j);
                CCell cell(sheet, ind, "");
                sheet.insert(move(pair<CIndex, CCell*>(CIndex(i,j), cell.replace_with(new CCellError(move(cell), "#NODATA")))));
            }
            continue;
        }
        string subtext;
        int bfound = 0;
        int efound = 0;
        for (int j = 0; j < m; ++j)
        {
            CIndex ind(i, j);
            if (efound == string::npos)
            {
                CCell cell(sheet, ind, "");
                sheet.insert(move(pair<CIndex, CCell*>(ind, cell.replace_with(new CCellError(move(cell), "#NODATA")))));
                continue;
            }

            efound = str.find_first_of("\t", bfound);
            if (efound != string::npos)
                subtext = str.substr(bfound, efound - bfound);
            else
                subtext = str.substr(bfound);

            CCell cell(sheet, ind, subtext);
            sheet.insert(move(pair<CIndex, CCell*>(ind, cell.replace_with(new CCellUndefined(move(cell))))));

            if (efound != string::npos)
                bfound = efound + 1;
        }
    }


    for (CSheet::iterator it = sheet.begin(); it != sheet.end(); ++it)
    {
        it->second->calculate();
    }

    for (int i = 0; i < n - 1; ++i)
    {
        for (int j = 0; j < m - 1; ++j)
        {
            cout << sheet[CIndex(i,j)] << '\t';
        }
        cout << sheet[CIndex(i,m - 1)] << '\n';
    }
    cout << sheet[CIndex(n - 1,m - 1)];

	return 0;
}
