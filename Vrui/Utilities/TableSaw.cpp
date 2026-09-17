/***********************************************************************
TableSaw - Utility to process tables read from CSV files.
Copyright (c) 2022-2026 Oliver Kreylos
***********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <Misc/StdError.h>
#include <IO/OpenFile.h>
#include <IO/CSVSource.h>

typedef std::vector<std::string> Row;
typedef std::vector<Row> Table;

Table createTable(size_t numRows,size_t numColumns,bool initRows)
	{
	/* Create the result table: */
	Table result;
	result.reserve(numRows);
	for(size_t row=0;row<numRows;++row)
		{
		/* Create the new row: */
		Row newRow;
		newRow.reserve(numColumns);
		
		/* Fill rows with empty strings if requested: */
		if(initRows)
			{
			for(size_t column=0;column<numColumns;++column)
				newRow.push_back(std::string());
			}
		
		/* Add the new row to the result table: */
		result.push_back(newRow);
		}
	
	return result;
	}

Table readTable(const char* fileName)
	{
	/* Open the given file as a CSV file: */
	IO::CSVSource file(IO::openFile(fileName));
	
	/* Read every record: */
	Table table;
	size_t maxRowSize=0;
	do
		{
		/* Read the current record: */
		Row current;
		do
			{
			/* Read the next field as a string: */
			std::string field=file.readField<std::string>();
			
			/* Replace "n.d." values with proper null values: */
			current.push_back(field!="n.d."&&field!="N.D."?field:std::string());
			}
		while(!file.eor());
		
		/* Store the current record in the result table: */
		if(maxRowSize<current.size())
			maxRowSize=current.size();
		table.push_back(current);
		}
	while(!file.eof());
	
	std::cout<<"Read table with "<<maxRowSize<<" columns and "<<table.size()<<" rows from input file "<<fileName<<std::endl;
	
	/* Pad all table rows to the maximum length: */
	for(Table::iterator tIt=table.begin();tIt!=table.end();++tIt)
		while(tIt->size()<maxRowSize)
			tIt->push_back(std::string());
	
	/* Return the result table: */
	return table;
	}

bool isNumber(const std::string& string)
	{
	const char* sPtr=string.c_str();
	
	/* Skip optional plus or minus sign: */
	if(*sPtr=='+'||*sPtr=='-')
		++sPtr;
	
	bool haveDigits=false;
	
	/* Skip integer digits: */
	for(;*sPtr>='0'&&*sPtr<='9';++sPtr)
		haveDigits=true;
	
	/* Check for decimal point: */
	if(*sPtr=='.')
		{
		++sPtr;
		
		/* Skip fractional digits: */
		for(;*sPtr>='0'&&*sPtr<='9';++sPtr)
			haveDigits=true;
		}
	
	if(!haveDigits)
		return false;
	
	/* Check for exponent indicator: */
	if(*sPtr=='e'||*sPtr=='E')
		{
		++sPtr;
		
		/* Skip optional plus or minus sign: */
		if(*sPtr=='+'||*sPtr=='-')
			++sPtr;
		
		haveDigits=false;
		
		/* Skip exponent digits: */
		for(;*sPtr>='0'&&*sPtr<='9';++sPtr)
			haveDigits=true;
		
		if(!haveDigits)
			return false;
		}
	
	return *sPtr=='\0';
	}

std::ostream& printValue(std::ostream& os,const std::string& value)
	{
	/* Check if the value needs to be quoted: */
	bool needsQuotes=false;
	for(std::string::const_iterator vIt=value.begin();vIt!=value.end()&&!needsQuotes;++vIt)
		needsQuotes=*vIt==','||isspace(*vIt);
	
	/* Print the value: */
	if(needsQuotes)
		os<<'"';
	os<<value;
	if(needsQuotes)
		os<<'"';
	
	return os;
	}

void writeTable(const Table& table,const char* fileName)
	{
	/* Open the output file: */
	std::ofstream file(fileName);
	
	/* Write all table rows: */
	for(Table::const_iterator rIt=table.begin();rIt!=table.end();++rIt)
		{
		/* Write the current row's columns: */
		Row::const_iterator cIt=rIt->begin();
		printValue(file,*cIt);
		for(++cIt;cIt!=rIt->end();++cIt)
			{
			file<<',';
			printValue(file,*cIt);
			}
		file<<std::endl;
		}
	
	std::cout<<"Wrote table with "<<table.front().size()<<" columns and "<<table.size()<<" rows"<<std::endl;
	}

Table& appendColumn(Table& table,const std::string& newHeader,const std::string& newValue)
	{
	/* Append a column of the given value to the table: */
	table[0].push_back(newHeader);
	for(size_t row=1;row<table.size();++row)
		table[row].push_back(newValue);
	
	return table;
	}

Table& appendColumns(Table& table,const Table& source,size_t startColumn,size_t endColumn)
	{
	/* Append columns from the source table to the destination table: */
	for(size_t row=0;row<table.size();++row)
		for(size_t column=startColumn;column<endColumn;++column)
			table[row].push_back(source[row][column]);
	
	return table;
	}

size_t findColumnIndex(const Table& table,const std::string& columnName)
	{
	for(size_t column=0;column<table.front().size();++column)
		if(table.front()[column]==columnName)
			return column;
	
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Column \"%s\" not found",columnName.c_str());
	}

size_t parseColumnIndex(const Table& table,const char* cPtr,const char** cEnd =0)
	{
	if(*cPtr=='\0')
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Empty column identifer");
	
	/* Check if the given string starts with a quote: */
	size_t numColumns=table.front().size();
	size_t result=numColumns;
	if(*cPtr=='"')
		{
		/* Collect characters until the closing quote: */
		std::string columnName;
		for(++cPtr;*cPtr!='\0'&&*cPtr!='"';++cPtr)
			columnName.push_back(*cPtr);
		
		/* Skip the closing quote: */
		if(*cPtr!='"')
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unterminated column name");
		++cPtr;
		
		/* Find the index of the column whose header matches the extracted name: */
		result=findColumnIndex(table,columnName);
		}
	else if(*cPtr>='0'&&*cPtr<='9')
		{
		/* Extract a column index: */
		result=0;
		for(;*cPtr>='0'&&*cPtr<='9';++cPtr)
			result=result*10+size_t(*cPtr-'0');
		}
	else
		{
		/* Collect characters until the next comma or end of string: */
		std::string columnName;
		for(;*cPtr!='\0'&&*cPtr!=',';++cPtr)
			columnName.push_back(*cPtr);
		
		/* Find the index of the column whose header matches the extracted name: */
		result=findColumnIndex(table,columnName);
		}
	
	if(cEnd!=0)
		*cEnd=cPtr;
	
	if(result>=numColumns)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Column index out of range");
	
	return result;
	}

Table& removeColumn(Table& table,size_t column)
	{
	/* Drop the column from each table row: */
	for(size_t row=0;row<table.size();++row)
		table[row].erase(table[row].begin()+column);
	
	return table;
	}

Table& moveColumn(Table& table,size_t column,size_t newColumn)
	{
	/* Move the column in each table row: */
	for(size_t row=0;row<table.size();++row)
		{
		/* Move the column by swapping neighbours forward or backward, which will prevent copying: */
		size_t col=column;
		while(col<newColumn)
			{
			std::swap(table[row][col],table[row][col+1]);
			++col;
			}
		while(col>newColumn)
			{
			std::swap(table[row][col],table[row][col-1]);
			--col;
			}
		}
	
	return table;
	}

Table selectColumns(const Table& table,const std::vector<size_t>& columnIndices)
	{
	/* Create an uninitialized result table: */
	Table result(createTable(table.size(),columnIndices.size(),false));
	
	/* Copy all requested columns from the source table: */
	for(std::vector<size_t>::const_iterator ciIt=columnIndices.begin();ciIt!=columnIndices.end();++ciIt)
		for(size_t row=0;row<table.size();++row)
			result[row].push_back(table[row][*ciIt]);
	
	return result;
	}

Table& appendRows(Table& table,const Table& source)
	{
	/* Append non-header rows from the second table to the first table: */
	table.reserve(table.size()+source.size()-1);
	for(Table::const_iterator rIt=source.begin()+1;rIt!=source.end();++rIt)
		table.push_back(*rIt);
	
	return table;
	}

struct IndexedString // Helper structure to sort tables by a row or column
	{
	/* Elements: */
	public:
	size_t index;
	const std::string* string;
	
	/* Constructors and destructors: */
	IndexedString(size_t sIndex,const std::string& sString)
		:index(sIndex),string(&sString)
		{
		}
	};

class IndexedStringComp // Comparison functor for indexed strings
	{
	/* Methods: */
	public:
	bool operator()(const IndexedString& is1,const IndexedString& is2) const
		{
		return *is1.string<*is2.string;
		}
	static int comp(const IndexedString& is1,const IndexedString& is2)
		{
		return is1.string->compare(*is2.string);
		}
	};

std::vector<IndexedString> indexRow(const Table& table,size_t row,size_t columnStart,size_t columnEnd)
	{
	/* Extract the given row from the source table: */
	std::vector<IndexedString> result;
	result.reserve(columnEnd-columnStart);
	for(size_t column=columnStart;column<columnEnd;++column)
		result.push_back(IndexedString(column,table[row][column]));
	
	/* Sort the index row: */
	std::sort(result.begin(),result.end(),IndexedStringComp());
	
	return result;
	}

std::vector<IndexedString> indexColumn(const Table& table,size_t column,size_t rowStart,size_t rowEnd)
	{
	/* Extract the given column from the source table: */
	std::vector<IndexedString> result;
	result.reserve(rowEnd-rowStart);
	for(size_t row=rowStart;row<rowEnd;++row)
		result.push_back(IndexedString(row,table[row][column]));
	
	/* Sort the index column: */
	std::sort(result.begin(),result.end(),IndexedStringComp());
	
	return result;
	}

Table columnUnion(const Table& table0,size_t startColumn0,size_t endColumn0,const Table& table1,size_t startColumn1,size_t endColumn1)
	{
	/* Index both tables' header rows: */
	std::vector<IndexedString> headers0=indexRow(table0,0,startColumn0,endColumn0);
	std::vector<IndexedString> headers1=indexRow(table1,0,startColumn1,endColumn1);
	
	/* Create a new uninitialized table of the same size as the first table: */
	Table result(createTable(table0.size(),table0.front().size(),false));
	
	/* Copy columns from the first table before the start column: */
	appendColumns(result,table0,0,startColumn0);
	
	/* Merge table columns: */
	size_t numCols0=endColumn0-startColumn0;
	size_t numCols1=endColumn1-startColumn1;
	size_t col0=0;
	size_t col1=0;
	while(col0<numCols0||col1<numCols1)
		{
		/* Compare the two column headers: */
		int comp;
		if(col0>=numCols0)
			comp=1;
		else if(col1>=numCols1)
			comp=-1;
		else
			comp=IndexedStringComp::comp(headers0[col0],headers1[col1]);
		
		/* Add the column with the smaller header: */
		if(comp<=0)
			{
			/* Next column comes from the first table: */
			appendColumns(result,table0,headers0[col0].index,headers0[col0].index+1);
			++col0;
			if(comp==0)
				++col1;
			}
		else
			{
			/* Next column takes its header from the second table: */
			appendColumn(result,table1[0][headers1[col1].index],std::string());
			++col1;
			}
		}
	
	/* Copy columns from the first table after the end column: */
	appendColumns(result,table0,endColumn0,table0.front().size());
	
	return result;
	}

Table columnIntersection(const Table& table0,size_t startColumn0,size_t endColumn0,const Table& table1,size_t startColumn1,size_t endColumn1)
	{
	/* Index both tables' header rows: */
	std::vector<IndexedString> headers0=indexRow(table0,0,startColumn0,endColumn0);
	std::vector<IndexedString> headers1=indexRow(table1,0,startColumn1,endColumn1);
	
	/* Create a new uninitialized table of the same size as the first table: */
	Table result(createTable(table0.size(),table0.front().size(),false));
	
	/* Copy columns from the first table before the start column: */
	appendColumns(result,table0,0,startColumn0);
	
	/* Merge table columns: */
	size_t numCols0=endColumn0-startColumn0;
	size_t numCols1=endColumn1-startColumn1;
	size_t col0=0;
	size_t col1=0;
	while(col0<numCols0&&col1<numCols1)
		{
		/* Compare the two column headers: */
		int comp=IndexedStringComp::comp(headers0[col0],headers1[col1]);
		
		if(comp==0)
			appendColumns(result,table0,headers0[col0].index,headers0[col0].index+1);
		if(comp<=0)
			++col0;
		if(comp>=0)
			++col1;
		}
	
	/* Copy columns from the first table after the end column: */
	appendColumns(result,table0,endColumn0,table0.front().size());
	
	return result;
	}

Row joinRow(const Table& table0,size_t row0,size_t joinColumn0,const Table& table1,size_t row1,size_t joinColumn1,bool retainJoinColumn)
	{
	/* Create the result row: */
	Row result;
	size_t numCols0=table0.front().size();
	size_t numCols1=table1.front().size();
	size_t numResultCols=numCols0-1+numCols1-1;
	if(retainJoinColumn)
		++numResultCols;
	result.reserve(numResultCols);
	
	/* Add the join column to the result table if so instructed: */
	if(retainJoinColumn)
		{
		if(row0<table0.size())
			result.push_back(table0[row0][joinColumn0]);
		else
			result.push_back(table1[row1][joinColumn1]);
		}
	
	if(row0<table0.size())
		{
		/* Add columns from the first table except the join column: */
		const Row& r0=table0[row0];
		for(size_t column=0;column<numCols0;++column)
			if(column!=joinColumn0)
				result.push_back(r0[column]);
		}
	else
		{
		/* Add null values for the columns from the first table except the join column: */
		for(size_t column=1;column<numCols0;++column)
			result.push_back(std::string());
		}
	
	if(row1<table1.size())
		{
		/* Add columns from the second table except the join column: */
		const Row& r1=table1[row1];
		for(size_t column=0;column<numCols1;++column)
			if(column!=joinColumn1)
				result.push_back(r1[column]);
		}
	else
		{
		/* Add null values for the columns from the second table except the join column: */
		for(size_t column=1;column<numCols1;++column)
			result.push_back(std::string());
		}
	
	return result;
	}

Table join(const Table& table0,size_t joinColumn0,bool outer0,const Table& table1,size_t joinColumn1,bool outer1,bool retainJoinColumn)
	{
	/* Index both tables' joining columns: */
	size_t numRows0=table0.size();
	std::vector<IndexedString> joinCol0=indexColumn(table0,joinColumn0,1,numRows0);
	size_t numRows1=table1.size();
	std::vector<IndexedString> joinCol1=indexColumn(table1,joinColumn1,1,numRows1);
	
	/* Create the result table and its header row: */
	Table result;
	result.push_back(joinRow(table0,0,joinColumn0,table1,0,joinColumn1,retainJoinColumn));
	
	/* Add all common rows from the two tables to the result table: */
	size_t row0=0;
	size_t row1=0;
	while(row0<numRows0-1||row1<numRows1-1)
		{
		/* Compare the two join column entries: */
		int comp;
		if(row0>=numRows0-1)
			comp=1;
		else if(row1>=numRows1-1)
			comp=-1;
		else
			comp=IndexedStringComp::comp(joinCol0[row0],joinCol1[row1]);
		
		/* Skip (or add if outer) the row with the smaller join column value: */
		if(comp<0)
			{
			if(outer0)
				result.push_back(joinRow(table0,joinCol0[row0].index,joinColumn0,table1,table1.size(),joinColumn1,retainJoinColumn));
			++row0;
			}
		else if(comp>0)
			{
			if(outer1)
				result.push_back(joinRow(table0,table0.size(),joinColumn0,table1,joinCol1[row1].index,joinColumn1,retainJoinColumn));
			++row1;
			}
		else
			{
			result.push_back(joinRow(table0,joinCol0[row0].index,joinColumn0,table1,joinCol1[row1].index,joinColumn1,retainJoinColumn));
			++row0;
			++row1;
			}
		}
	
	return result;
	}

Table subtract(const Table& table0,size_t joinColumn0,const Table& table1,size_t joinColumn1)
	{
	/* Index both tables' joining columns: */
	size_t numRows0=table0.size();
	std::vector<IndexedString> joinCol0=indexColumn(table0,joinColumn0,1,numRows0);
	size_t numRows1=table1.size();
	std::vector<IndexedString> joinCol1=indexColumn(table1,joinColumn1,1,numRows1);
	
	/* Create the result table and its header row: */
	Table result;
	result.push_back(table0.front());
	
	/* Add all rows in the first table not in the second table to the result table: */
	size_t row0=0;
	size_t row1=0;
	while(row0<numRows0-1||row1<numRows1-1)
		{
		/* Compare the two join column entries: */
		int comp;
		if(row0>=numRows0-1)
			comp=1;
		else if(row1>=numRows1-1)
			comp=-1;
		else
			comp=IndexedStringComp::comp(joinCol0[row0],joinCol1[row1]);
		
		/* Retain the first table's row if it comes before the second table's row: */
		if(comp<0)
			result.push_back(table0[joinCol0[row0].index]);
		
		if(comp<=0)
			++row0;
		if(comp>=0)
			++row1;
		}
	
	return result;
	}

int main(int argc,char* argv[])
	{
	/* Create a stack of tables: */
	std::vector<Table> tables;
	
	/* Process the command line: */
	for(int argi=1;argi<argc;++argi)
		{
		/* Retrieve the top two tables from the stack: */
		size_t numRows0=0;
		size_t numCols0=0;
		Table* table0=0;
		if(tables.size()>=2)
			{
			table0=&tables[tables.size()-2];
			numRows0=table0->size();
			numCols0=table0->front().size();
			}
		size_t numRows1=0;
		size_t numCols1=0;
		Table* table1=0;
		if(tables.size()>=1)
			{
			table1=&tables[tables.size()-1];
			numRows1=table1->size();
			numCols1=table1->front().size();
			}
		
		/* Process the next command: */
		if(strcasecmp(argv[argi],"swap")==0)
			{
			/* Swap the two top tables: */
			std::swap(*table0,*table1);
			}
		else if(strcasecmp(argv[argi],"printHeaders")==0)
			{
			/* Print the column names of the top table: */
			Row::iterator cIt=table1->front().begin();
			printValue(std::cout,*cIt);
			for(++cIt;cIt!=table1->front().end();++cIt)
				{
				std::cout<<',';
				printValue(std::cout,*cIt);
				}
			std::cout<<std::endl;
			}
		else if(strcasecmp(argv[argi],"renameColumn")==0)
			{
			size_t columnIndex=parseColumnIndex(*table1,argv[++argi]);
			table1->front()[columnIndex]=argv[++argi];
			}
		else if(strcasecmp(argv[argi],"moveColumn")==0)
			{
			size_t columnIndex=parseColumnIndex(*table1,argv[++argi]);
			size_t destinationIndex=atoi(argv[++argi]);
			moveColumn(*table1,columnIndex,destinationIndex);
			
			std::cout<<"Moved column for table with "<<table1->front().size()<<" columns and "<<table1->size()<<" rows"<<std::endl;
			}
		else if(strcasecmp(argv[argi],"removeColumn")==0)
			{
			size_t columnIndex=parseColumnIndex(*table1,argv[++argi]);
			removeColumn(*table1,columnIndex);
			
			std::cout<<"Removed column for table with "<<table1->front().size()<<" columns and "<<table1->size()<<" rows"<<std::endl;
			}
		else if(strcasecmp(argv[argi],"appendColumns")==0)
			{
			Table table=*table0;
			appendColumns(table,*table1,0,numCols1);
			tables.pop_back();
			tables.pop_back();
			tables.push_back(table);
			}
		else if(strcasecmp(argv[argi],"printColumn")==0)
			{
			size_t columnIndex=parseColumnIndex(*table1,argv[++argi]);
			printValue(std::cout,(*table1)[1][columnIndex]);
			for(size_t row=2;row<table1->size();++row)
				{
				std::cout<<',';
				printValue(std::cout,(*table1)[row][columnIndex]);
				}
			std::cout<<std::endl;
			}
		else if(strcasecmp(argv[argi],"save")==0)
			{
			/* Save the top table from the stack: */
			writeTable(*table1,argv[++argi]);
			
			tables.pop_back();
			}
		else if(strcasecmp(argv[argi],"select")==0)
			{
			/* Read a list of column names or indices: */
			std::vector<size_t> columnIndices;
			const char* cPtr=argv[++argi];
			while(*cPtr!='\0')
				{
				/* Get the next column index: */
				columnIndices.push_back(parseColumnIndex(*table1,cPtr,&cPtr));
				
				/* Check if there is another column index: */
				if(*cPtr!='\0'&&*cPtr!=',')
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Malformed select column list");
				if(*cPtr==',')
					++cPtr;
				}
			
			/* Select columns from the table: */
			Table selectedTable=selectColumns(*table1,columnIndices);
			std::cout<<"Selected table with "<<selectedTable.front().size()<<" columns and "<<selectedTable.size()<<" rows"<<std::endl;
			
			tables.pop_back();
			tables.push_back(selectedTable);
			}
		else if(strcasecmp(argv[argi],"mergeUnion")==0)
			{
			size_t numHeaderCols=atoi(argv[++argi]);
			
			/* Expand both tables such that their header rows match afterwards: */
			Table table0Extended=columnUnion(*table0,numHeaderCols,numCols0,*table1,numHeaderCols,numCols1);
			Table table1Extended=columnUnion(*table1,numHeaderCols,numCols1,*table0,numHeaderCols,numCols0);
			
			/* Add the second extended table's rows to the first extended table: */
			appendRows(table0Extended,table1Extended);
			std::cout<<"Merged table with "<<table0Extended.front().size()<<" columns and "<<table0Extended.size()<<" rows"<<std::endl;
			
			tables.pop_back();
			tables.pop_back();
			tables.push_back(table0Extended);
			}
		else if(strcasecmp(argv[argi],"mergeIntersection")==0)
			{
			size_t numHeaderCols=atoi(argv[++argi]);
			
			/* Shrink both tables such that their header rows match afterwards: */
			Table table0Shrunk=columnIntersection(*table0,numHeaderCols,numCols0,*table1,numHeaderCols,numCols1);
			Table table1Shrunk=columnIntersection(*table1,numHeaderCols,numCols1,*table0,numHeaderCols,numCols0);
			
			/* Add the second shrunk table's rows to the first shrunk table: */
			appendRows(table0Shrunk,table1Shrunk);
			std::cout<<"Merged table with "<<table0Shrunk.front().size()<<" columns and "<<table0Shrunk.size()<<" rows"<<std::endl;
			
			tables.pop_back();
			tables.pop_back();
			tables.push_back(table0Shrunk);
			}
		else if(strcasecmp(argv[argi],"join")==0||
		        strcasecmp(argv[argi],"innerJoin")==0||
		        strcasecmp(argv[argi],"leftOuterJoin")==0||
		        strcasecmp(argv[argi],"rightOuterJoin")==0||
		        strcasecmp(argv[argi],"outerJoin")==0)
			{
			/* Retrieve join parameters: */
			bool joinOuters[2]={false,false};
			if(strcasecmp(argv[argi],"leftOuterJoin")==0)
				joinOuters[0]=true;
			else if(strcasecmp(argv[argi],"rightOuterJoin")==0)
				joinOuters[1]=true;
			else if(strcasecmp(argv[argi],"outerJoin")==0)
				joinOuters[1]=joinOuters[0]=true;
			size_t joinColumns[2];
			joinColumns[0]=parseColumnIndex(*table0,argv[++argi]);
			joinColumns[1]=parseColumnIndex(*table1,argv[++argi]);
			bool retainJoinColumn=strcasecmp(argv[++argi],"true")==0;
			
			/* Join the two tables: */
			Table joinedTable=join(*table0,joinColumns[0],joinOuters[0],*table1,joinColumns[1],joinOuters[1],retainJoinColumn);
			std::cout<<"Joined table with "<<joinedTable.front().size()<<" columns and "<<joinedTable.size()<<" rows"<<std::endl;
			
			tables.pop_back();
			tables.pop_back();
			tables.push_back(joinedTable);
			}
		else if(strcasecmp(argv[argi],"subtract")==0)
			{
			/* Retrieve subtract parameters: */
			size_t joinColumns[2];
			joinColumns[0]=parseColumnIndex(*table0,argv[++argi]);
			joinColumns[1]=parseColumnIndex(*table1,argv[++argi]);
			
			/* Subtract the two tables: */
			Table subtractedTable=subtract(*table0,joinColumns[0],*table1,joinColumns[1]);
			std::cout<<"Subtracted table with "<<subtractedTable.front().size()<<" columns and "<<subtractedTable.size()<<" rows"<<std::endl;
			
			tables.pop_back();
			tables.pop_back();
			tables.push_back(subtractedTable);
			}
		else
			{
			/* Load a table onto the stack: */
			tables.push_back(readTable(argv[argi]));
			}
		}
	
	return 0;
	}
