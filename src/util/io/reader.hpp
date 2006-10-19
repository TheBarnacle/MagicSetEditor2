//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2006 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#ifndef HEADER_UTIL_IO_READER
#define HEADER_UTIL_IO_READER

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <wx/txtstrm.h>

template <typename T> class Defaultable;
template <typename T> class Scriptable;

// ----------------------------------------------------------------------------- : Reader

typedef wxInputStream  InputStream;
typedef shared_ptr<wxInputStream> InputStreamP;

/// The Reader can be used for reading (deserializing) objects
/** This class makes use of the reflection functionality, in effect
 *  an object tells the Reader what fields it would like to read.
 *  The reader then sees if the requested field is currently available.
 *
 *  The handle functions ensure that afterwards the reader is at the line after the
 *  object that was just read.
 */
class Reader {
  public:
	/// Construct a reader that reads from the given input stream
	/** filename is used only for error messages
	 */
	Reader(const InputStreamP& input, String filename = _(""));
	
	/// Construct a reader that reads a file in a package
	Reader(String filename);
	
	/// Tell the reflection code we are reading
	inline bool reading() const { return true; }
	/// Is the thing currently being read 'complex', i.e. does it have children
	inline bool isComplex() const { return value.empty(); }
	
	// --------------------------------------------------- : Handling objects
	/// Handle an object: read it if it's name matches
	template <typename T>
	void handle(const Char* name, T& object) {
		if (enterBlock(name)) {
			handle(object);
			exitBlock();
		}
	}
	/// Reads a vector from the input stream
	template <typename T>
	void handle(const Char* name, vector<T>& vector);
	
	/// Reads an object of type T from the input stream
	template <typename T> void handle(T& object);
	/// Reads a shared_ptr from the input stream
	template <typename T> void handle(shared_ptr<T>& pointer);
	/// Reads a map from the input stream
	template <typename K, typename V> void handle(map<K,V>& map);
	/// Reads a Defaultable from the input stream
	template <typename T> void handle(Defaultable<T>&);
	/// Reads a Scriptable from the input stream
	template <typename T> void handle(Scriptable<T>&);
	
  private:
	// --------------------------------------------------- : Data
	/// The line we read
	String line;
	/// The key and value of the last line we read
	String key, value;
	/// A string spanning multiple lines
	String multi_line_str;
	/// Indentation of the last line we read
	int indent;
	/// Indentation of the block we are in
	int expected_indent;
	/// Did we just open a block (i.e. not read any more lines of it)?
	bool just_opened;
	
	/// Filename for error messages
	String filename;
	/// Line number for error messages
	UInt line_number;
	/// Input stream we are reading from
	InputStreamP input;
	/// Text stream wrapping the input stream
	wxTextInputStream stream;
	
	// --------------------------------------------------- : Reading the stream
	
	/// Is there a block with the given key under the current cursor?
	bool enterBlock(const Char* name);
	/// Leave the block we are in
	void exitBlock();
	
	/// Move to the next non empty line
	void moveNext();
	/// Reads the next line from the input, and stores it in line/key/value/indent
	void readLine();
	
	/// Issue a warning: "Unexpected key: $key"
	void unexpected();
};

// ----------------------------------------------------------------------------- : Container types

/// Construct a new type, possibly reading something in the process.
/** By default just creates a new object.
 *  This function can be overloaded to provide different behaviour
 */
template <typename T>
shared_ptr<T> read_new(Reader& reader) {
	return new_shared<T>();
}

template <typename T>
void Reader::handle(const Char* name, vector<T>& vector) {
	String vectorKey = singular_form(name);
	while (enterBlock(vectorKey)) {
		vector.resize(vector.size() + 1);
		handle(vector.back());
		exitBlock();
	}
}

template <typename T>
void Reader::handle(shared_ptr<T>& pointer) {
	if (!pointer) pointer = read_new<T>(*this);
	handle(*pointer);
}

template <typename K, typename V>
void Reader::handle(map<K,V>& map) {
	// TODO
}

// ----------------------------------------------------------------------------- : Reflection

/// Implement reflection as used by Reader
#define REFLECT_OBJECT_READER(Cls)								\
	template<> void Reader::handle<Cls>(Cls& object) {			\
		while (indent >= expected_indent) {						\
			UInt l = line_number;								\
			object.reflect(*this);								\
			if (l == line_number) {								\
				/* error */										\
				do {											\
					moveNext();									\
				} while (indent > expected_indent);				\
			}													\
		}														\
	}															\
	void Cls::reflect(Reader& reader) {							\
		reflect_impl(reader);									\
	}

// ----------------------------------------------------------------------------- : Reflection for enumerations

/// Implement enum reflection as used by Reader
#define REFLECT_ENUM_READER(Enum)								\
	template<> void Reader::handle<Enum>(Enum& enum_) {			\
		EnumReader reader(value);								\
		reflect_ ## Enum(enum_, reader);						\
		if (!reader.isDone()) {									\
			/* warning message */								\
		}														\
	}

/// 'Tag' to be used when reflecting enumerations for Reader
class EnumReader {
  public:
	inline EnumReader(String read)
		: read(read), first(true), done(false) {}
	
	/// Handle a possible value for the enum, if the name matches the name in the input
	template <typename Enum>
	inline void handle(const Char* name, Enum value, Enum& enum_) {
		if (!done && read == name) {
			first = true;
			enum_ = value;
		} else if (first) {
			first = false;
			enum_ = value;
		}
	}
	
	inline bool isDone() const { return done; }
	
  private:
	String read;  ///< The string to match to a value name
	bool   first; ///< Has the first (default) value been matched?
	bool   done;  ///< Was anything matched?
};

// ----------------------------------------------------------------------------- : EOF
#endif