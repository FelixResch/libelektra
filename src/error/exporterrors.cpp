/**
 * @file
 *
 * @brief
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 */

#include "parser.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

#ifndef _WIN32
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

using namespace std;

static ostream & printKDBErrors (ostream & os, parse_t & p)
{
	os << "/*This is an auto-generated file generated by exporterrors. Do not modify it.*/" << endl
	   << endl
	   << "#ifndef KDBERRORS_H" << endl
	   << "#define KDBERRORS_H" << endl
	   << endl
	   << "#include <kdb.h>" << endl
	   << "#include <kdbhelper.h>" << endl
	   << "#include <kdblogger.h>" << endl
	   << "#include <kdbmacros.h>" << endl
	   << "#include <string.h>" << endl
	   << endl
	   << "#ifdef __cplusplus" << endl
	   << "using namespace ckdb;" << endl
	   << "#endif" << endl;
	os << endl << endl;

	for (size_t i = 1; i < p.size (); ++i)
	{
		os << "#define ELEKTRA_SET_" << p[i]["macro"] << "_ERROR(key, text) \\" << endl
		   << "\tdo { ELEKTRA_LOG (\"Add Error " << p[i]["number"] << ": %s\", text); elektraSetError" << p[i]["number"]
		   << "(key, text, __FILE__, ELEKTRA_STRINGIFY(__LINE__), ELEKTRA_STRINGIFY(ELEKTRA_MODULE_NAME)); } while (0)" << endl;
		os << "#define ELEKTRA_SET_" << p[i]["macro"] << "_ERRORF(key, text, ...) \\" << endl
		   << "\tdo { ELEKTRA_LOG (\"Add Error " << p[i]["number"] << ": \" text, __VA_ARGS__); elektraSetErrorf" << p[i]["number"]
		   << "(key, text, __FILE__, ELEKTRA_STRINGIFY(__LINE__), ELEKTRA_STRINGIFY(ELEKTRA_MODULE_NAME), __VA_ARGS__); } while (0)"
		   << endl;

		os << "#define ELEKTRA_ADD_" << p[i]["macro"] << "_WARNING(key, text) \\" << endl
		   << "\tdo { ELEKTRA_LOG (\"Add Warning " << p[i]["number"] << ": %s\", text); elektraAddWarning" << p[i]["number"]
		   << "(key, text, __FILE__, ELEKTRA_STRINGIFY(__LINE__), ELEKTRA_STRINGIFY(ELEKTRA_MODULE_NAME)); } while (0)" << endl;
		os << "#define ELEKTRA_ADD_" << p[i]["macro"] << "_WARNINGF(key, text, ...) \\" << endl
		   << "\tdo { ELEKTRA_LOG (\"Add Warning " << p[i]["number"] << ": \" text, __VA_ARGS__); elektraAddWarningf"
		   << p[i]["number"]
		   << "(key, text, __FILE__, ELEKTRA_STRINGIFY(__LINE__), ELEKTRA_STRINGIFY(ELEKTRA_MODULE_NAME), __VA_ARGS__); } while (0)"
		   << endl;

		os << endl;
	}
	os << endl;

	for (size_t i = 1; i < p.size (); ++i)
	{
		if (p[i]["unused"] == "yes")
		{
			continue;
		}

		if (p[i]["macro"].empty ())
		{
			throw invalid_argument ("Error with number " + p[i]["number"] + " does not have a mandatory macro.");
		}

		os << "#define ELEKTRA_WARNING_" << p[i]["macro"] << " \"" << p[i]["number"] << "\"" << endl;
		os << "#define ELEKTRA_ERROR_" << p[i]["macro"] << " \"" << p[i]["number"] << "\"" << endl;
	}

	os << endl << endl;

	for (size_t i = 1; i < p.size (); ++i)
	{

		for (int f = 0; f < 2; ++f)
		{
			if (f == 0)
			{
				os << "static inline void elektraAddWarningf" << p[i]["number"] << "(Key *warningKey, const char *reason,"
				   << endl
				   << "	const char *file, const char *line, const char *module, ...)  __attribute__ ((format (printf, 2, "
				      "6)));"
				   << endl;
				os << "static inline void elektraAddWarningf" << p[i]["number"] << "(Key *warningKey, const char *reason,"
				   << endl
				   << "	const char *file, const char *line, const char *module, ...)" << endl;
			}
			else
			{
				os << "static inline void elektraAddWarning" << p[i]["number"] << "(Key *warningKey, const char *reason,"
				   << endl
				   << "	const char *file, const char *line, const char *module)" << endl;
			}
			os << "{" << endl
			   << "	if (!warningKey) return;" << endl
			   << "" << endl
			   << "	char buffer[25] = \"warnings/#00\";buffer[12] = '\\0';" << endl
			   << "	const Key *meta = keyGetMeta(warningKey, \"warnings\");" << endl
			   << "	if (meta)" << endl
			   << "	{" << endl
			   << "		buffer[10] = keyString(meta)[0];" << endl
			   << "		buffer[11] = keyString(meta)[1];" << endl
			   << "		buffer[11]++;" << endl
			   << "		if (buffer[11] > '9')" << endl
			   << "		{" << endl
			   << "			buffer[11] = '0';" << endl
			   << "			buffer[10]++;" << endl
			   << "			if (buffer[10] > '9') buffer[10] = '0';" << endl
			   << "		}" << endl
			   << "		keySetMeta(warningKey, \"warnings\", &buffer[10]);" << endl
			   << "	} else  keySetMeta(warningKey, \"warnings\", \"00\");" << endl
			   << "" << endl
			   << "	keySetMeta(warningKey, buffer, \"number description  module file line function reason\");" << endl
			   << "	strcat(buffer, \"/number\" );" << endl
			   << "	keySetMeta(warningKey, buffer, \"" << p[i]["number"] << "\");" << endl
			   << "	buffer[12] = '\\0'; strcat(buffer, \"/description\");" << endl
			   << "	keySetMeta(warningKey, buffer, \"" << p[i]["description"] << "\");" << endl
			   << "	buffer[12] = '\\0'; strcat(buffer, \"/module\");"
			   << endl
			   // TODO: Not the best implementation, fix if better way is found
			   << "	if (strcmp(module, \"ELEKTRA_MODULE_NAME\") == 0) module = \"kdb\";" << endl
			   << "	keySetMeta(warningKey, buffer, module);" << endl
			   << "	buffer[12] = '\\0'; strcat(buffer, \"/file\");" << endl // should be called sourcefile
			   << "	keySetMeta(warningKey, buffer, file);" << endl
			   << "	buffer[12] = '\\0'; strcat(buffer, \"/line\");" << endl
			   << "	keySetMeta(warningKey, buffer, line);" << endl
			   << "	buffer[12] = '\\0'; strcat(buffer, \"/mountpoint\");" << endl
			   << "	keySetMeta(warningKey, buffer, keyName(warningKey));" << endl
			   << "	buffer[12] = '\\0'; strcat(buffer, \"/configfile\");" << endl
			   << "	keySetMeta(warningKey, buffer, keyString(warningKey));" << endl
			   << "	buffer[12] = '\\0'; strcat(buffer, \"/reason\");" << endl;
			if (f == 0)
			{
				os << "	va_list arg;" << endl
				   << "	va_start(arg, module);" << endl
				   << "	char * r = elektraVFormat(reason, arg);" << endl
				   << "	keySetMeta(warningKey, buffer, r);" << endl
				   << "	elektraFree(r);" << endl
				   << "	va_end(arg);" << endl;
			}
			else
			{
				os << "	keySetMeta(warningKey, buffer, reason);" << endl;
			}
			os << "}" << endl << endl;
		}

		for (int f = 0; f < 2; ++f)
		{
			if (f == 0)
			{
				os << "static inline void elektraSetErrorf" << p[i]["number"] << "(Key *errorKey, const char *reason,"
				   << endl
				   << "	const char *file, const char *line, const char *module, ...)  __attribute__ ((format (printf, 2, "
				      "6)));"
				   << endl
				   << "static inline void elektraSetErrorf" << p[i]["number"] << "(Key *errorKey, const char *reason,"
				   << endl
				   << "	const char *file, const char *line, const char *module, ...)" << endl;
			}
			else
			{
				os << "static inline void elektraSetError" << p[i]["number"] << "(Key *errorKey, const char *reason,"
				   << endl
				   << "	const char *file, const char *line, const char *module)" << endl;
			}
			os << "{" << endl
			   << "	if (!errorKey) return;" << endl
			   << "	char buffer[25] = \"warnings/#00\";" << endl
			   << " 	const Key *meta = keyGetMeta(errorKey, \"error\");" << endl
			   << "	if (meta)" << endl
			   << "	{" << endl
			   << "		const Key *warningMeta = keyGetMeta(errorKey, \"warnings\");" << endl
			   << "		if (warningMeta)" << endl
			   << "		{" << endl
			   << "			buffer[10] = keyString(warningMeta)[0];" << endl
			   << "			buffer[11] = keyString(warningMeta)[1];" << endl
			   << "			buffer[11]++;" << endl
			   << "			if (buffer[11] > '9')" << endl
			   << "			{" << endl
			   << "				buffer[11] = '0';" << endl
			   << "				buffer[10]++;" << endl
			   << "				if (buffer[10] > '9') buffer[10] = '0';" << endl
			   << "			}" << endl
			   << "			keySetMeta(errorKey, \"warnings\", &buffer[10]);" << endl
			   << "		} else	keySetMeta(errorKey, \"warnings\", \"00\");" << endl
			   << "		keySetMeta(errorKey, buffer, \"number description  module file line function "
			      "reason\");"
			   << endl
			   << "		strcat(buffer, \"/number\" );" << endl
			   << "		keySetMeta(errorKey, buffer, \"" << p[i]["number"] << "\");" << endl
			   << "		buffer[12] = '\\0'; strcat(buffer, \"/description\");" << endl
			   << "		keySetMeta(errorKey, buffer, \"" << p[i]["description"] << "\");" << endl
			   << "		buffer[12] = '\\0'; strcat(buffer, \"/module\");"
			   << endl
			   // TODO: Not the best implementation, fix if better way is found
			   << "		if (strcmp(module, \"ELEKTRA_MODULE_NAME\") == 0) module = \"kdb\";" << endl
			   << "		keySetMeta(errorKey, buffer, module);" << endl
			   << "		buffer[12] = '\\0'; strcat(buffer, \"/file\");" << endl // should be called sourcefile
			   << "		keySetMeta(errorKey, buffer, file);" << endl
			   << "		buffer[12] = '\\0'; strcat(buffer, \"/line\");" << endl
			   << "		keySetMeta(errorKey, buffer, line);" << endl
			   << "		buffer[12] = '\\0'; strcat(buffer, \"/mountpoint\");" << endl
			   << "		keySetMeta(errorKey, buffer, keyName(errorKey));" << endl
			   << "		buffer[12] = '\\0'; strcat(buffer, \"/configfile\");" << endl
			   << "		keySetMeta(errorKey, buffer, keyString(errorKey));" << endl
			   << "		buffer[12] = '\\0'; strcat(buffer, \"/reason\");" << endl
			   << "	}" << endl
			   << " 	else" << endl
			   << " 	{" << endl
			   << "		keySetMeta(errorKey, \"error\", \""
			   << "number description  module file line function reason"
			   << "\");" << endl
			   << "		keySetMeta(errorKey, \"error/number\", \"" << p[i]["number"] << "\");" << endl
			   << "		keySetMeta(errorKey, \"error/description\", \"" << p[i]["description"] << "\");"
			   << endl
			   // TODO: Not the best implementation, fix if better way is found
			   << "		if (strcmp(module, \"ELEKTRA_MODULE_NAME\") == 0) module = \"kdb\";" << endl
			   << "		keySetMeta(errorKey, \"error/module\", module);" << endl
			   << "		keySetMeta(errorKey, \"error/file\", "
			   << "file"
			   << ");" << endl
			   << "		keySetMeta(errorKey, \"error/line\", "
			   << "line"
			   << ");" << endl
			   << "		keySetMeta(errorKey, \"error/mountpoint\", "
			   << "keyName(errorKey)"
			   << ");" << endl
			   << "		keySetMeta(errorKey, \"error/configfile\", "
			   << "keyString(errorKey)"
			   << ");" << endl
			   << " 	}" << endl;
			if (f == 0)
			{
				os << "	va_list arg;" << endl
				   << "	va_start(arg, module);" << endl
				   << "	char * r = elektraVFormat(reason, arg);" << endl
				   << " 	if (meta)" << endl
				   << "			keySetMeta(errorKey, buffer, r);" << endl
				   << " 	else" << endl
				   << "			keySetMeta(errorKey, \"error/reason\", "
				   << "r"
				   << ");" << endl
				   << "	elektraFree(r);" << endl
				   << "	va_end(arg);" << endl;
			}
			else
			{
				os << " 	if (meta)" << endl
				   << "			keySetMeta(errorKey, buffer, reason);" << endl
				   << " 	else" << endl
				   << "			keySetMeta(errorKey, \"error/reason\", reason);" << endl;
			}
			os << "}" << endl << endl;
		}
	}

	os << "static inline KeySet *elektraErrorSpecification (void)" << endl
	   << "{" << endl
	   << "	return ksNew (30," << endl
	   << "		keyNew (\"system/elektra/modules/error/specification\"," << endl
	   << "			KEY_VALUE, \"the specification of all error codes\", KEY_END)," << endl;
	for (size_t i = 1; i < p.size (); ++i)
	{
		os << "		keyNew (\"system/elektra/modules/error/specification/" << p[i]["number"] << "\"," << endl
		   << "			KEY_END)," << endl
		   << "		keyNew (\"system/elektra/modules/error/specification/" << p[i]["number"] << "/description\"," << endl
		   << "			KEY_VALUE, \"" << p[i]["description"] << "\", KEY_END)," << endl;
		//		   << "		keyNew (\"system/elektra/modules/error/specification/" << p[i]["number"] << "/severity\","
		//<<
		// endl
		//		   << "			KEY_VALUE, \"" << p[i]["severity"] << "\", KEY_END)," << endl
		//		   << "		keyNew (\"system/elektra/modules/error/specification/" << p[i]["number"] << "/module\"," <<
		// endl
		//		   << "			KEY_VALUE, \"" << p[i]["module"] << "\", KEY_END)," << endl;
	}
	os << "		KS_END);" << endl << "}" << endl;

	os << "static inline void elektraTriggerWarnings (const char *nr, Key *parentKey, const char *message)" << endl << "{" << endl;
	for (size_t i = 1; i < p.size (); ++i)
	{
		os << "	if (strcmp(nr, \"" << p[i]["number"] << "\") == 0)" << endl << "	{" << endl;
		os << "		ELEKTRA_ADD_" << p[i]["macro"] << "_WARNING (parentKey, message);" << endl
		   << "		return;" << endl
		   << "	}" << endl;
	}
	os << "	ELEKTRA_ADD_INTERNAL_WARNINGF (parentKey, \"Unkown warning code %s\", nr);" << endl << "}" << endl << "" << endl;
	os << "static inline void elektraTriggerError (const char *nr, Key *parentKey, const char *message)" << endl << "{" << endl;
	for (size_t i = 1; i < p.size (); ++i)
	{
		os << "	if (strcmp(nr, \"" << p[i]["number"] << "\") == 0)" << endl << "	{" << endl;
		os << "		ELEKTRA_SET_" << p[i]["macro"] << "_ERROR (parentKey, message);" << endl
		   << "		return;" << endl
		   << "	}" << endl;
	}
	os << "	ELEKTRA_SET_INTERNAL_ERRORF (parentKey, \"Unkown error code %s\", nr);" << endl << "}" << endl;

	os << "#endif" << endl;
	return os;
}

static string macroCaseToPascalCase (const string & s)
{
	std::string result;
	result.resize (s.size ());
	auto upcase = true;
	std::transform (s.begin (), s.end (), result.begin (), [&upcase](char c) {
		int x = upcase ? toupper (c) : tolower (c);
		upcase = c == '_';
		return x;
	});
	result.erase (std::remove (result.begin (), result.end (), '_'), result.end ());
	return result;
}

static ostream & printHighlevelErrorsHeader (ostream & os, parse_t & p)
{
	os << "/*This is an auto-generated file generated by exporterrors highlevel. Do not modify it.*/" << endl
	   << endl
	   << "#ifndef ELEKTRA_ERRORS_H" << endl
	   << "#define ELEKTRA_ERRORS_H" << endl
	   << endl
	   << "#include <elektra/error.h>" << endl
	   << "#include <elektra/types.h>" << endl
	   << endl
	   << "#ifdef __cplusplus" << endl
	   << "extern \"C\" {" << endl
	   << "#endif" << endl;

	for (size_t i = 1; i < p.size (); ++i)
	{
		const auto & macroName = p[i]["macro"];
		os << "#define ELEKTRA_ERROR_" << macroName << "_ERROR(description, ...) elektraError" << macroName
		   << "Error (\"TODO\", __FILE__, __LINE__)" << endl;
		os << "#define ELEKTRA_ERROR_ADD_" << macroName << "_WARNING(error, description, ...) elektraErrorAdd" << macroName
		   << "Warning (error, \"TODO\", __FILE__, __LINE__)" << endl;

		os << "ElektraError * elektraError" << macroCaseToPascalCase (macroName)
		   << "Error (const char * module, const char * file, kdb_long_t line, const char * description, ...);" << endl;
		os << "void elektraErrorAdd" << macroCaseToPascalCase (macroName)
		   << "Warning (ElektraError * error, const char * module, const char * file, kdb_long_t line, const char * description, "
		      "...);"
		   << endl;

		os << endl;
	}

	os << "#ifdef __cplusplus" << endl << "}" << endl << "#endif" << endl;

	os << "#endif // ELEKTRA_ERRORS_PRIVATE_H" << endl;
	os << endl;
	return os;
}

static ostream & printHighlevelErrorsSource (ostream & os, parse_t & p, const std::string & header)
{
	os << "/*This is an auto-generated file generated by exporterrors_highlevel. Do not modify it.*/" << endl
	   << endl
	   << "#include <" << header << ">" << endl
	   << "#include <kdbprivate.h>" << endl
	   << "#include <kdbhelper.h>" << endl
	   << "#include <kdberrors.h>" << endl
	   << endl;

	// work-around not needed after new error concept
	os << "#if defined(__GNUC__)" << endl
	   << "#pragma GCC diagnostic push" << endl
	   << "#pragma GCC diagnostic ignored \"-Wformat\"" << endl
	   << "#endif" << endl
	   << endl;

	for (size_t i = 1; i < p.size (); ++i)
	{
		const auto & macroName = p[i]["macro"];

		os << "ElektraError * elektraError" << macroCaseToPascalCase (macroName)
		   << "Error (const char * module, const char * file, kdb_long_t line, const char * description, ...) {" << endl
		   << "\tva_list arg;" << endl
		   << "\tva_start (arg, description);" << endl
		   << "\tchar * descriptionText = elektraVFormat (description, arg);" << endl
		   << "\tElektraError * error = elektraErrorCreate (ELEKTRA_ERROR_" << p[i]["macro"]
		   << ", descriptionText, module, file, line);" << endl
		   << "\telektraFree (descriptionText);" << endl
		   << "\tva_end (arg);" << endl
		   << "\treturn error;" << endl
		   << "}" << endl
		   << endl;

		os << "void elektraErrorAdd" << macroCaseToPascalCase (macroName)
		   << "Warning (ElektraError * error, const char * module, const char * file, kdb_long_t line, const char * description, "
		      "...) {"
		   << endl
		   << "\tva_list arg;" << endl
		   << "\tva_start (arg, description);" << endl
		   << "\tchar * descriptionText = elektraVFormat (description, arg);" << endl
		   << "\tElektraError * warning = elektraErrorCreate (ELEKTRA_ERROR_" << p[i]["macro"]
		   << ", descriptionText, module, file, line);" << endl
		   << "\telektraFree (descriptionText);" << endl
		   << "\tva_end (arg);" << endl
		   << "\telektraErrorAddWarning (error, warning);" << endl
		   << "}" << endl
		   << endl;
	}

	os << "#if defined(__GNUC__)" << endl << "#pragma GCC diagnostic pop" << endl << "#endif" << endl << endl;

	return os;
}

template <typename Func>
static int writeFile (parse_t & data, const char * filename, Func printFn)
{
	std::string tmpfile = filename;
#ifndef _WIN32
	tmpfile += ".tmp";
	tmpfile += to_string (getpid ());
#endif
	{
		ofstream fout (tmpfile);
		if (!fout.is_open ())
		{
			cerr << "Could not open output file " << filename << endl;
			return 1;
		}
		printFn (fout, data);
	}

#ifndef _WIN32
	int fd = open (tmpfile.c_str (), O_RDWR);
	if (fd == -1)
	{
		cerr << "Could not reopen file " << filename << endl;
		return 2;
	}
	if (fsync (fd) == -1)
	{
		cerr << "Could not fsync config file " << filename << " because ", strerror (errno);
		close (fd);
		return 3;
	}
	close (fd);

	if (rename (tmpfile.c_str (), filename) == -1)
	{
		cerr << "Could not rename file " << tmpfile << " to " << filename << endl;
		return 4;
	}
#endif
	return 0;
}

static void printUsage (const std::string & name)
{
	cerr << "Usage " << name << "MODE infile [options]"
	     << "\tMODES:" << endl
	     << "\t\tkdb\t\t options: outfile" << endl
	     << "\t\thighlevel\t\t options: outcodes outpublic outprivate outsource includepublic includeprivate" << endl
	     << "\t\t\t outcodes: outfile for error codes enum" << endl
	     << "\t\t\t outpublic: outfile for public header" << endl
	     << "\t\t\t outprivate: outfile for private header" << endl
	     << "\t\t\t outsource: outfile for source code" << endl
	     << "\t\t\t includepublic: include path (including file name) for public header" << endl
	     << "\t\t\t includeprivate: include path (including file name) for private header" << endl
	     << endl;
}

int main (int argc, char ** argv) try
{
	if (argc < 2)
	{
		printUsage (argv[0]);
		return 1;
	}

	std::string mode = argv[1];

	if (mode == "kdb")
	{
		switch (argc)
		{
		case 3:
		{
			string infile = argv[2];
			parse_t data = parse (infile);
			printKDBErrors (cout, data);
			break;
		}
		case 4:
		{
			string infile = argv[2];
			parse_t data = parse (infile);
			writeFile (data, argv[3], printKDBErrors);
			break;
		}
		default:
			printUsage (argv[0]);
			return 1;
		}
	}
	else if (mode == "highlevel")
	{
		switch (argc)
		{
		case 3:
		{
			string infile = argv[2];
			parse_t data = parse (infile);
			printHighlevelErrorsHeader (cout, data);
			cout << endl << endl;
			printHighlevelErrorsSource (cout, data, "");
			break;
		}
		case 6:
		{
			string infile = argv[2];
			parse_t data = parse (infile);
			writeFile (data, argv[3], printHighlevelErrorsHeader);

			auto func = [&](ostream & os, parse_t & p) { printHighlevelErrorsSource (os, p, argv[5]); };

			writeFile (data, argv[4], func);
			break;
		}
		default:
			printUsage (argv[0]);

			return 1;
		}
	}
	else
	{
		cerr << "Unknown mode" << endl;
		printUsage (argv[0]);
		return 1;
	}


	return 0;
}
catch (parse_error const & e)
{
	cerr << "The line " << e.linenr << " caused following parse error: " << e.info << endl;
	return 2;
}
