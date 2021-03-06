/*
 * Copyright (C) John Baier 2014 <ebusd@johnm.de>
 *
 * This file is part of ebusd.
 *
 * ebusd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ebusd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ebusd. If not, see http://www.gnu.org/licenses/.
 */

#ifndef LIBEBUS_DATA_H_
#define LIBEBUS_DATA_H_

#include "symbol.h"
#include "result.h"
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>

using namespace std;

/** the separator character used between fields (in CSV only). */
#define FIELD_SEPARATOR ','

/** the separator character used between multiple values (in CSV only). */
#define VALUE_SEPARATOR ';'

/** the separator character used between base type name and length (in CSV only). */
#define LENGTH_SEPARATOR ':'

/** the replacement string for undefined values (in UI and CSV). */
#define NULL_VALUE "-"

/** the separator character used between fields (in UI only). */
#define UI_FIELD_SEPARATOR ';'

/** the message part in which a data field is stored. */
enum PartType {
	pt_any,          // stored in any data (master or slave)
	pt_masterData,   // stored in master data
	pt_slaveData,    // stored in slave data
	};

/** the available base data types. */
enum BaseType {
	bt_str,    // text string in a @a StringDataField
	bt_hexstr, // hex digit string in a @a StringDataField
	bt_dat,    // date in a @a StringDataField
	bt_tim,    // time in a @a StringDataField
	bt_num,    // numeric value in a @a NumericDataField
};

/** flags for dataType_t. */
const unsigned int ADJ = 0x01; // adjustable length, numBits is maximum length
const unsigned int BCD = 0x02; // binary representation is BCD
const unsigned int REV = 0x04; // reverted binary representation (most significant byte first)
const unsigned int SIG = 0x08; // signed value
const unsigned int LST = 0x10; // value list is possible (without applied divisor)
const unsigned int DAY = 0x20; // forced value list defaulting to week days
const unsigned int IGN = 0x40; // ignore value during read and write

/** the structure for defining field types with their properties. */
typedef struct {
	const char* name;                        // field identifier
	const unsigned int maxBits;              // number of bits (maximum length if @a ADJ flag is set, must be multiple of 8 with flag @a BCD)
	const BaseType type;                     // base data type
	const unsigned int flags;                // flags (e.g. @a BCD)
	const unsigned int replacement;          // replacement value (fill-up value for @a bt_str / @a bt_hexstr, no replacement if equal to @a minValueOrLength for @a bt_num)
	const unsigned int minValueOrLength;     // minimum binary value (minimum length of string for @a StringDataField)
	const unsigned int maxValueOrLength;     // maximum binary value (maximum length of string for @a StringDataField)
	const unsigned int divisor;              // @a bt_number: divisor
	const unsigned char precisionOrFirstBit; // @a bt_number: precision for formatting or offset to first bit if (@a numBits%8)!=0
} dataType_t;


/**
 * @brief Parse an unsigned int value.
 * @param str the string to parse.
 * @param base the numerical base.
 * @param minValue the minimum resulting value.
 * @param maxValue the maximum resulting value.
 * @param result the variable in which to store an error code when parsing failed or the value is out of bounds.
 * @param length the optional variable in which to store the number of read characters.
 */
unsigned int parseInt(const char* str, int base, const unsigned int minValue, const unsigned int maxValue, result_t& result, unsigned int* length=NULL);

/**
 * @brief Print the error position of the iterator to stdout.
 * @param begin the iterator to the beginning of the items.
 * @param end the iterator to the end of the items.
 * @param pos the iterator with the erroneous position.
 * @param separator the character to place between items.
 */
void printErrorPos(vector<string>::iterator begin, const vector<string>::iterator end, vector<string>::iterator pos);


class DataFieldTemplates;
class SingleDataField;

/**
 * @brief Base class for all kinds of data fields.
 */
class DataField
{
public:

	/**
	 * @brief Constructs a new instance.
	 * @param name the field name.
	 * @param comment the field comment.
	 */
	DataField(const string name, const string comment)
		: m_name(name), m_comment(comment) {}
	/**
	 * @brief Destructor.
	 */
	virtual ~DataField() {}
	/**
	 * @brief Factory method for creating new instances.
	 * @param it the iterator to traverse for the definition parts.
	 * @param end the iterator pointing to the end of the definition parts.
	 * @param templates the @a DataFieldTemplates to be referenced by name, or NULL.
	 * @param returnField the variable in which to store the created instance.
	 * @param isSetMessage whether the field is part of a set message (default false).
	 * @param dstAddress the destination bus address (default @a SYN for creating a template @a DataField).
	 * @return @a RESULT_OK on success, or an error code.
	 * Note: the caller needs to free the created instance.
	 */
	static result_t create(vector<string>::iterator& it, const vector<string>::iterator end,
			DataFieldTemplates* templates, DataField*& returnField,
			const bool isSetMessage=false, const unsigned char dstAddress=SYN);
	/**
	 * @brief Returns the length of this field (or contained fields) in bytes.
	 * @param partType the message part of the contained fields to limit the length calculation to.
	 * @return the length of this field (or contained fields) in bytes.
	 */
	virtual unsigned char getLength(PartType partType) = 0;
	/**
	 * @brief Derives a new DataField from this field.
	 * @param name the field name.
	 * @param comment the field comment, or empty to use this fields comment.
	 * @param unit the value unit, or empty to use this fields unit (if applicable).
	 * @param partType the message part in which the field is stored.
	 * @param divisor the extra divisor to apply on the value, or 1 for none (if applicable).
	 * @param values the value=text assignments, or empty to use this fields assignments (if applicable).
	 * @param fields the @a vector to which created @a SingleDataField instances shall be added.
	 */
	virtual result_t derive(string name, string comment,
			string unit, const PartType partType,
			unsigned int divisor, map<unsigned int, string> values,
			vector<SingleDataField*>& fields) = 0;
	/**
	 * @brief Get the field name.
	 * @return the field name.
	 */
	string getName() const { return m_name; }
	/**
	 * @brief Get the field comment.
	 * @return the field comment.
	 */
	string getComment() const { return m_comment; }
	/**
	 * @brief Dump the field settings to the output.
	 * @param output the @a ostream to dump to.
	 */
	virtual void dump(ostream& output) = 0;
	/**
	 * @brief Reads the value from the @a SymbolString.
	 * @param partType the @a PartType of the data.
	 * @param data the unescaped data @a SymbolString for reading binary data.
	 * @param offset the additional offset to add for reading binary data.
	 * @param output the @a ostringstream to append the formatted value to.
	 * @param leadingSeparator whether to prepend a separator before the formatted value.
	 * @param verbose whether to prepend the name, append the unit (if present), and append
	 * the comment in square brackets (if present).
	 * @param separator the separator character between multiple fields.
	 * @return @a RESULT_OK on success (or if the partType does not match), or an error code.
	 */
	virtual result_t read(const PartType partType,
			SymbolString& data, unsigned char offset,
			ostringstream& output, bool leadingSeparator=false,
			bool verbose=false, char separator=UI_FIELD_SEPARATOR) = 0;
	/**
	 * @brief Writes the value to the master or slave @a SymbolString.
	 * @param input the @a istringstream to parse the formatted value from.
	 * @param partType the @a PartType of the data.
	 * @param data the unescaped data @a SymbolString for writing binary data.
	 * @param offset the additional offset to add for writing binary data.
	 * @param separator the separator character between multiple fields.
	 * @return @a RESULT_OK on success, or an error code.
	 */
	virtual result_t write(istringstream& input,
			const PartType partType, SymbolString& data,
			unsigned char offset, char separator=UI_FIELD_SEPARATOR) = 0;

protected:

	/** the field name. */
	const string m_name;
	/** the field comment. */
	const string m_comment;

};


/**
 * @brief A single DataField.
 */
class SingleDataField : public DataField
{
public:

	/**
	 * @brief Constructs a new instance.
	 * @param name the field name.
	 * @param comment the field comment.
	 * @param unit the value unit.
	 * @param dataType the data type definition.
	 * @param partType the message part in which the field is stored.
	 * @param length the number of symbols in the message part in which the field is stored.
	 */
	SingleDataField(const string name, const string comment,
			const string unit, const dataType_t dataType, const PartType partType,
			const unsigned char length)
		: DataField(name, comment),
		  m_unit(unit), m_dataType(dataType), m_partType(partType),
		  m_length(length) {}
	/**
	 * @brief Destructor.
	 */
	virtual ~SingleDataField() {}
	/**
	 * @brief Get the value unit.
	 * @return the value unit.
	 */
	string getUnit() const { return m_unit; }
	/**
	 * @brief Get whether this field is ignored.
	 * @return whether this field is ignored.
	 */
	bool isIgnored() const { return (m_dataType.flags & IGN) != 0; }
	/**
	 * @brief Get the message part in which the field is stored.
	 * @return the message part in which the field is stored.
	 */
	PartType getPartType() const { return m_partType; }
	// @copydoc
	virtual unsigned char getLength(PartType partType) { return partType == m_partType ? m_length : 0; };
	// re-use same position as previous field as not all bits of fully consumed yet
	/**
	 * @brief Get whether this field uses a full byte offset.
	 * @param after @p true to check after consuming the bits, false to check before.
	 * @return true if this field uses a full byte offset, false if this field
	 * only consumes a part of a byte and a subsequent field may re-use the same offset.
	 */
	virtual bool hasFullByteOffset(bool after) { return true; }
	// @copydoc
	virtual void dump(ostream& output);
	// @copydoc
	virtual result_t read(const PartType partType,
			SymbolString& data, unsigned char offset,
			ostringstream& output, bool leadingSeparator=false,
			bool verbose=false, char separator=UI_FIELD_SEPARATOR);
	// @copydoc
	virtual result_t write(istringstream& input,
			const PartType partType, SymbolString& data,
			unsigned char offset, char separator=UI_FIELD_SEPARATOR);

protected:

	/**
	 * @brief Internal method for reading the field from a @a SymbolString.
	 * @param input the unescaped @a SymbolString to read the binary value from.
	 * @param offset the offset in the @a SymbolString.
	 * @param output the ostringstream to append the formatted value to.
	 * @return @a RESULT_OK on success, or an error code.
	 */
	virtual result_t readSymbols(SymbolString& input, const unsigned char offset, ostringstream& output) = 0;
	/**
	 * @brief Internal method for writing the field to a @a SymbolString.
	 * @param input the @a istringstream to parse the formatted value from.
	 * @param offset the offset in the @a SymbolString.
	 * @param output the unescaped @a SymbolString to write the binary value to.
	 * @return @a RESULT_OK on success, or an error code.
	 */
	virtual result_t writeSymbols(istringstream& input, const unsigned char offset, SymbolString& output) = 0;

protected:

	/** the value unit. */
	const string m_unit;
	/** the data type definition. */
	const dataType_t m_dataType;
	/** the message part in which the field is stored. */
	const PartType m_partType;
	/** the number of symbols in the message part in which the field is stored. */
	const unsigned char m_length;

};


/**
 * @brief Base class for all string based data fields.
 */
class StringDataField : public SingleDataField
{
public:

	/**
	 * @brief Constructs a new instance.
	 * @param name the field name.
	 * @param comment the field comment.
	 * @param unit the value unit.
	 * @param dataType the data type definition.
	 * @param partType the message part in which the field is stored.
	 * @param length the number of symbols in the message part in which the field is stored.
	 */
	StringDataField(const string name, const string comment,
			const string unit, const dataType_t dataType, const PartType partType,
			const unsigned char length)
		: SingleDataField(name, comment, unit, dataType, partType, length) {}
	/**
	 * @brief Destructor.
	 */
	virtual ~StringDataField() {}
	// @copydoc
	virtual result_t derive(string name, string comment,
			string unit, const PartType partType,
			unsigned int divisor, map<unsigned int, string> values,
			vector<SingleDataField*>& fields);
	// @copydoc
	virtual void dump(ostream& output);

protected:

	// @copydoc
	virtual result_t readSymbols(SymbolString& input, const unsigned char offset, ostringstream& output);
	// @copydoc
	virtual result_t writeSymbols(istringstream& input, const unsigned char offset, SymbolString& output);

};


/**
 * @brief Base class for all numeric data fields.
 */
class NumericDataField : public SingleDataField
{
public:

	/**
	 * @brief Constructs a new instance.
	 * @param name the field name.
	 * @param comment the field comment.
	 * @param unit the value unit.
	 * @param dataType the data type definition.
	 * @param partType the message part in which the field is stored.
	 * @param length the number of symbols in the message part in which the field is stored.
	 * @param bitCount the number of bits in the binary value.
	 * @param bitOffset the offset to the first bit in the binary value.
	 */
	NumericDataField(const string name, const string comment,
			const string unit, const dataType_t dataType, const PartType partType,
			const unsigned char length, const unsigned char bitCount, const unsigned char bitOffset)
		: SingleDataField(name, comment, unit, dataType, partType, length),
		  m_bitCount(bitCount), m_bitOffset(bitOffset) {}
	/**
	 * @brief Destructor.
	 */
	virtual ~NumericDataField() {}
	// @copydoc
	virtual bool hasFullByteOffset(bool after);
	// @copydoc
	virtual void dump(ostream& output);

protected:

	/**
	 * @brief Internal method for reading the raw value from a @a SymbolString.
	 * @param input the unescaped @a SymbolString to read the binary value from.
	 * @param offset the offset in the @a SymbolString.
	 * @param value the variable in which to store the raw value.
	 * @return @a RESULT_OK on success, or an error code.
	 */
	result_t readRawValue(SymbolString& input, const unsigned char offset, unsigned int& value);
	/**
	 * @brief Internal method for writing the raw value to a @a SymbolString.
	 * @param value the raw value to write.
	 * @param offset the offset in the @a SymbolString.
	 * @param output the unescaped @a SymbolString to write the binary value to.
	 * @return @a RESULT_OK on success, or an error code.
	 */
	result_t writeRawValue(unsigned int value, const unsigned char offset, SymbolString& output);

	/** the number of bits in the binary value. */
	const unsigned char m_bitCount;

	/** the offset to the first bit in the binary value. */
	const unsigned char m_bitOffset;

};


/**
 * @brief Base class for all numeric data fields with a number representation.
 */
class NumberDataField : public NumericDataField
{
public:

	/**
	 * @brief Constructs a new instance.
	 * @param name the field name.
	 * @param comment the field comment.
	 * @param unit the value unit.
	 * @param dataType the data type definition.
	 * @param partType the message part in which the field is stored.
	 * @param length the number of symbols in the message part in which the field is stored.
	 * @param divisor the extra divisor to apply on the value, or 1 for none.
	 */
	NumberDataField(const string name, const string comment,
			const string unit, const dataType_t dataType, const PartType partType,
			const unsigned char length, const unsigned char bitCount,
			const unsigned int divisor)
		: NumericDataField(name, comment, unit, dataType, partType, length, bitCount,
				(dataType.maxBits < 8) ? dataType.precisionOrFirstBit : 0),
		m_divisor(divisor) {}
	/**
	 * @brief Destructor.
	 */
	virtual ~NumberDataField() {}
	// @copydoc
	virtual result_t derive(string name, string comment,
			string unit, const PartType partType,
			unsigned int divisor, map<unsigned int, string> values,
			vector<SingleDataField*>& fields);
	// @copydoc
	virtual void dump(ostream& output);

protected:

	// @copydoc
	virtual result_t readSymbols(SymbolString& input, const unsigned char offset, ostringstream& output);
	// @copydoc
	virtual result_t writeSymbols(istringstream& input, const unsigned char offset, SymbolString& output);

private:

	/** the combined divisor to apply on the value, or 1 for none. */
	const unsigned int m_divisor;

};


/**
 * @brief A numeric data field with a list of value=text assignments and a string representation.
 */
class ValueListDataField : public NumericDataField
{
public:

	/**
	 * @brief Constructs a new instance.
	 * @param name the field name.
	 * @param comment the field comment.
	 * @param unit the value unit.
	 * @param dataType the data type definition.
	 * @param partType the message part in which the field is stored.
	 * @param length the number of symbols in the message part in which the field is stored.
	 * @param values the value=text assignments.
	 */
	ValueListDataField(const string name, const string comment,
			const string unit, const dataType_t dataType, const PartType partType,
			const unsigned char length, const unsigned char bitCount,
			const map<unsigned int, string> values)
		: NumericDataField(name, comment, unit, dataType, partType, length, bitCount,
				(dataType.maxBits < 8) ? dataType.precisionOrFirstBit : 0),
		m_values(values) {}
	/**
	 * @brief Destructor.
	 */
	virtual ~ValueListDataField() {}
	// @copydoc
	virtual result_t derive(string name, string comment,
			string unit, const PartType partType, unsigned int divisor,
			map<unsigned int, string> values,
			vector<SingleDataField*>& fields);
	// @copydoc
	virtual void dump(ostream& output);

protected:

	// @copydoc
	virtual result_t readSymbols(SymbolString& input, const unsigned char offset, ostringstream& output);
	// @copydoc
	virtual result_t writeSymbols(istringstream& input, const unsigned char offset, SymbolString& output);

private:

	/** the value=text assignments. */
	map<unsigned int, string> m_values;

};


/**
 * @brief A set of DataFields.
 */
class DataFieldSet : public DataField
{
public:

	/**
	 * @brief Create the @a DataFieldSet for parsing the identification message (service 0x07 0x04).
	 * @return the @a DataFieldSet for parsing the identification message.
	 */
	static DataFieldSet* createIdentFields();
	/**
	 * @brief Constructs a new instance.
	 * @param name the field name.
	 * @param comment the field comment.
	 * @param fields the @a vector of @a SingleDataField instances part of this set.
	 */
	DataFieldSet(const string name, const string comment,
			const vector<SingleDataField*> fields)
		: DataField(name, comment),
		  m_fields(fields) {}
	/**
	 * @brief Destructor.
	 */
	virtual ~DataFieldSet();
	// @copydoc
	virtual unsigned char getLength(PartType partType);
	// @copydoc
	virtual result_t derive(string name, string comment,
			string unit, const PartType partType,
			unsigned int divisor, map<unsigned int, string> values,
			vector<SingleDataField*>& fields);
	/**
	 * @brief Returns the @a SingleDataField at the specified index.
	 * @param index the index of the @a SingleDataField to return.
	 * @return the @a SingleDataField at the specified index, or NULL.
	 */
	SingleDataField* operator[](const size_t index) { if (index >= m_fields.size()) return NULL; return m_fields[index]; }
	/**
	 * @brief Returns the @a SingleDataField at the specified index.
	 * @param index the index of the @a SingleDataField to return.
	 * @return the @a SingleDataField at the specified index, or NULL.
	 */
	const SingleDataField* operator[](const size_t index) const { if (index >= m_fields.size()) return NULL; return m_fields[index]; }
	/**
	 * @brief Returns the number of @a SingleDataFields instances in this set.
	 * @return the number of available @a SingleDataField instances.
	 */
	size_t size() const { return m_fields.size(); }
	// @copydoc
	virtual void dump(ostream& output);
	// @copydoc
	virtual result_t read(const PartType partType,
			SymbolString& data, unsigned char offset,
			ostringstream& output, bool leadingSeparator=false,
			bool verbose=false, char separator=UI_FIELD_SEPARATOR);
	// @copydoc
	virtual result_t write(istringstream& input,
			const PartType partType, SymbolString& data,
			unsigned char offset, char separator=UI_FIELD_SEPARATOR);

private:

	/** the @a vector of @a SingleDataField instances part of this set. */
	vector<SingleDataField*> m_fields;

};


/**
 * @brief An abstract class that support reading definitions from a file.
 */
template<typename T>
class FileReader
{
public:

	/**
	 * @brief Constructs a new instance.
	 */
	FileReader(bool supportsDefaults)
			: m_supportsDefaults(supportsDefaults) {}
	/**
	 * @brief Destructor.
	 */
	virtual ~FileReader() {}
	/**
	 * @brief Reads the definitions from a file.
	 * @param filename the name (and path) of the file to read.
	 * @return @a RESULT_OK on success, or an error code.
	 */
	virtual result_t readFromFile(string filename, T arg=NULL)
	{
		ifstream ifs;
		ifs.open(filename.c_str(), ifstream::in);
		if (ifs.is_open() == false)
			return RESULT_ERR_NOTFOUND;

		string line;
		unsigned int lineNo = 0;
		vector<string> row;
		string token;
		vector< vector<string> > defaults;
		while (getline(ifs, line) != 0) {
			lineNo++;
			// skip empty lines and comments
			if (line.length() == 0 || line.substr(0, 1) == "#" || line.substr(0, 2) == "//")
				continue;
			istringstream isstr(line);
			row.clear();
			while (getline(isstr, token, FIELD_SEPARATOR) != 0)
				row.push_back(token);

			if (m_supportsDefaults == true && line.substr(0, 1) == "*") {
				row[0] = row[0].substr(1);
				defaults.push_back(row);
				continue;
			}
			result_t result = addFromFile(row, arg, m_supportsDefaults == true ? &defaults : NULL);
			if (result != RESULT_OK) {
				cerr << "error reading \"" << filename << "\" line " << static_cast<unsigned>(lineNo) << ": " << getResultCode(result) << endl;
				ifs.close();
				return result;
			}
		}

		ifs.close();
		return RESULT_OK;
	}
	/**
	 * @brief Adds a definition that was read from a file.
	 * @param row the definition row read from the file.
	 * @param defaults all previously read default rows (initial star char removed), or NULL if not supported.
	 * @return @a RESULT_OK on success, or an error code.
	 */
	virtual result_t addFromFile(vector<string>& row, T arg, vector< vector<string> >* defaults) = 0;

private:
	/** whether this instance supports rows with defaults (starting with a star). */
	bool m_supportsDefaults;

};


/**
 * @brief A map of template @a DataField instances.
 */
class DataFieldTemplates : public FileReader<void*>
{
public:

	/**
	 * @brief Constructs a new instance.
	 */
	DataFieldTemplates() : FileReader(false) {}
	/**
	 * @brief Destructor.
	 */
	virtual ~DataFieldTemplates() { clear(); }
	/**
	 * @brief Removes all @a DataField instances.
	 */
	void clear();
	/**
	 * @brief Adds a template @a DataField instance to this map.
	 * @param field the @a DataField instance to add.
	 * @param replace whether replacing an already stored instance is allowed.
	 * @return @a RESULT_OK on success, or an error code.
	 * Note: the caller may not free the added instance on success.
	 */
	result_t add(DataField* message, bool replace=false);
	// @copydoc
	virtual result_t addFromFile(vector<string>& row, void* arg, vector< vector<string> >* defaults);
	/**
	 * @brief Gets the template @a DataField instance with the specified name.
	 * @return the template @a DataField instance, or NULL.
	 * Note: the caller may not free the returned instance.
	 */
	DataField* get(string name);

private:

	/** the known template @a DataField instances by name. */
	map<string, DataField*> m_fieldsByName;

};

#endif // LIBEBUS_DATA_H_
