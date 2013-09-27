// ==========================================================================
//                 SeqAn - The Library for Sequence Analysis
// ==========================================================================
// Copyright (c) 2006-2013, Knut Reinert, FU Berlin
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Knut Reinert or the FU Berlin nor the names of
//       its contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL KNUT REINERT OR THE FU BERLIN BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// ==========================================================================
// Author: Stephan Aiche <stephan.aiche@fu-berlin.de>
// ==========================================================================

#ifndef SEQAN_CORE_INCLUDE_SEQAN_ARG_PARSE_ARG_PARSE_PARSE_H_
#define SEQAN_CORE_INCLUDE_SEQAN_ARG_PARSE_ARG_PARSE_PARSE_H_

#include <seqan/arg_parse/arg_parse_option.h>
#include <seqan/arg_parse/argument_parser.h>
#include <seqan/arg_parse/arg_parse_ctd_support.h>

namespace seqan {

// ----------------------------------------------------------------------------
// Function parse()
// ----------------------------------------------------------------------------

/*!
 * @fn ArgumentParser#parse
 * @headerfile <seqan/arg_parse.h>
 * @brief Parse command line parameters.
 *
 * @signature TResult parse(parser, argc, argv[, outStream, errStream]]);
 *
 * @param parser    The ArgumentParser to use for parsing and for storing parse results.
 * @param argc      The number of arguments (<tt>int</tt>).
 * @param argv      The arguments (<tt>const char * argv[]</tt>).
 * @param outStream The <tt>std::ostream</tt> to use for output.
 * @param errStream The <tt>std::ostream</tt> to use for error output.
 *
 * @return TResult The parse result, of type @link ArgumentParser::ParseResult @endlink.
 *
 * This function must be called before retrieving any options or arguments from the parser.
 */

/**
.Function.ArgumentParser#parse
..summary:Parses the command line.
..class:Class.ArgumentParser
..cat:Miscellaneous
..signature:parse(parser, argc, argv[, outputStream, errorStream])
..param.parser:The @Class.ArgumentParser@ object.
...type:Class.ArgumentParser
..param.argc:Count of the objects on the command line.
..param.argv:Array of the different command line arguments ($const char *argv[]$).
..param.errorStream:A stream where error messages are sent to.
..remarks:Must be called before retrieving options or arguments.
..returns:$true$ if all required arguments are set and parseable and neither the help nor version argument is set.
..include:seqan/arg_parse.h
*/

// Helper class for parsing command line arguments.
//
// Putting things into its a class allows us to structure the parsing in a fine way.

class ArgumentParserHelper_
{
public:
    typedef ArgumentParser::TArgumentMapSize TArgumentPosition;

    // Reference to the ArgumentParser to parse for.
    ArgumentParser & parser;
    // The argc and argv from the main() method.
    int argc;
    const char ** argv;

    // The parser's state is stored in the following variables.

    // The special argument "--" is ignored as an option and only arguments can follow.  The following flag holds the
    // state that this token was seen.
    bool seenDashDash;
    // The index of the current positional argument.
    TArgumentPosition currentArgument;

    ArgumentParserHelper_(ArgumentParser & parser, int argc, const char * argv[])
            : parser(parser), argc(argc), argv(argv), seenDashDash(false), currentArgument(0)
    {}

    void reset()
    {
        seenDashDash = false;
        currentArgument = 0;
    }

    // Perform the argument parsing.
    void parseArgs()
    {
        reset();  // reset state

        // Parse binary name from command line if it was not set.
        if (empty(getAppName(parser)))
            _parseAppName(parser, argv[0]);

        for (int argi = 1; argi < argc; ++argi)
        {
            if (seenDashDash || strlen(argv[argi]) == 0 || argv[argi][0] != '-')
                // Handle as position argument if we have seen "--" or does not start with dash.
                handleArgument(argv[argi]);
            else if (strcmp(argv[argi], "--") == 0)
                // If this is "--" then ignore the argument itself but set the seen flag for it.
                seenDashDash = true;
            else
                // This is an option.
                handleOption(argi, argv[argi]);
        }
    }

private:

    // Handle the given string as an argument.
    //
    // Throw ParseException in case of too many arguments.
    void handleArgument(char const * argStr)
    {
        // Check whether we have the largest number of allowed arguments already.
        if (parser.argumentList.size() <= currentArgument)
            throw ParseException("Too many arguments!");

        ArgParseArgument & argument = getArgument(parser, currentArgument);
        _assignArgumentValue(argument, argStr);

        if (!isListArgument(argument))
            ++currentArgument;
    }

    // Handle the given string as an option.
    //
    // Throw InvalidOptionException if there is a formal error (== "-") or invalid option naparser.
    //
    // Throw MissingArgumentException in case of --key=value option but the option requires multiple values.
    void handleOption(int & argi, std::string arg)
    {
        if (arg == "-")
            throw InvalidOptionException("-");

        if (arg[1] == '-')
            handleLongOption(argi, arg);
        else
            handleShortOption(argi, arg);
    }

    // Handle the given string as a long option.
    void handleLongOption(int & argi, std::string const & arg)
    {
        // We will store the option name and the value in these variables.
        std::string longOpt = arg;
        std::string val;

        // Split option in case of --key=value format.
        size_t t = arg.find('=');
        if (t != std::string::npos)  // is --key=value option
        {
            val = arg.substr(t + 1);
            longOpt = arg.substr(0, t);
        }
        longOpt.erase(0, 2);

        // Guard against invalid option names.
        if (!hasOption(parser, longOpt))
            throw InvalidOptionException(longOpt);

        // Parse out the values for the option from the command line arguments.
        ArgParseOption & opt = getOption(parser, longOpt);
        if (t != std::string::npos)  // was --key=value option
        {
            // We can only assign one value in this case.  If the option expected more than one value then this is an
            // error.
            if (numberOfAllowedValues(opt) != 1)
                throw MissingArgumentException(longOpt);
            // If everything is fine then we can assign the value to the option.
            _assignArgumentValue(opt, val);
        }
        else if (isBooleanOption(opt))
        {
            // Handling boolean options is simple.
            _assignArgumentValue(opt, "true");
        }
        else
        {
            // If we reach here, we have a --key value option and might require multiple values.

            // Guard against missing values.
            if (argi + (int)numberOfAllowedValues(opt) >= argc)
                throw MissingArgumentException(longOpt);
            // We have sufficient values, get them from argv and assign them to the option.
            for (int t = 0; t < (int)numberOfAllowedValues(opt); ++t)
                _assignArgumentValue(opt, argv[++argi]);
        }
    }

    // Handle the given string as a short option.
    void handleShortOption(int & argi, std::string const & arg)
    {
        // Parse out the short option.  This is made complicated because the user can concatenate multiple short options
        // into one.  NB: we do not check for and warn about possible ambiguities.
        //
        // We enumerate the possible option names starting from left to right, then sorted descendingly by length.
        // Boolean options can be squished together in this way without further restrictions.  If the option requires
        // additional arguments then it has to be the last in the short options.

        for (unsigned s = 1; s < arg.size(); ++s)
        {
            unsigned e = arg.size();
            for (; s < e; --e)
            {
                if (hasOption(parser, arg.substr(s, e - s)))
                {
                    ArgParseOption & opt = getOption(parser, arg.substr(s, e - s));
                    s = --e;  // advance in squished options;  s > e if at end

                    // Boolean options are easy to handle.
                    if (isBooleanOption(opt))
                    {
                        _assignArgumentValue(opt, "true");
                        continue;
                    }

                    // Handle option with values.
                    if (e < arg.size() - 1)
                    {
                        std::stringstream what;
                        what << "invalid combination of arguments -- " << arg << std::endl;
                        throw ParseException(what.str());
                    }

                    // Handle the case of too few options.
                    if (argi + (int)numberOfAllowedValues(opt) >= argc)
                        throw MissingArgumentException(opt.shortName);

                    // Assign the required option value.s
                    for (int t = 0; t < (int)numberOfAllowedValues(opt); ++t)
                        _assignArgumentValue(opt, argv[++argi]);
                }
            }

            // No option was found.
            if (s == e)
                throw InvalidOptionException(arg.substr(s));
        }
    }
};

// Parser driver function.
inline ArgumentParser::ParseResult parse(ArgumentParser & me,
                                         int argc,
                                         const char * argv[],
                                         std::ostream & outputStream,
                                         std::ostream & errorStream)
{
    try
    {
        // Perform the parsing without any valid value checking on the argument values.
        ArgumentParserHelper_ parserHelper(me, argc, argv);
        parserHelper.parseArgs();

        // Copy the file extensions from the "--${NAME}-file-ext" options to "--${NAME}".
        for (unsigned i = 0; i < me.optionMap.size(); ++i)
        {
            ArgParseOption & opt = me.optionMap[i];
            std::string longExtName = opt.longName + "-file-ext";
            std::string shortExtName = opt.shortName + "-file-ext";
            if (!opt.longName.empty() && hasOption(me, longExtName))
                opt._fileExtensions = getArgumentValues(getOption(me, longExtName));
            else if (!opt.shortName.empty() && hasOption(me, shortExtName))
                opt._fileExtensions = getArgumentValues(getOption(me, longExtName));
        }
        // Copy the file extensions from the "--arg-${NUM}-file-ext" options to the argument.
        for (unsigned i = 0; i < me.argumentList.size(); ++i)
        {
            ArgParseArgument & arg = me.argumentList[i];
            std::stringstream ss;
            ss << "arg-" << (i + 1) << "-file-ext";
            if (hasOption(me, ss.str()))
                arg._fileExtensions = getArgumentValues(getOption(me, ss.str()));
        }

        // Check all arguments for their values.
        for (unsigned i = 0; i < me.optionMap.size(); ++i)
            _checkValue(me.optionMap[i]);
        for (unsigned i = 0; i < me.argumentList.size(); ++i)
            _checkValue(me.argumentList[i]);
    }
    catch (ParseException & ex)
    {
        errorStream << getAppName(me) << ": " << ex.what() << std::endl;
        return ArgumentParser::PARSE_ERROR;
    }

    // Handle the special options.
    if (hasOption(me, "version") && isSet(me, "version"))
    {
        printVersion(me, outputStream);
        return ArgumentParser::PARSE_VERSION;
    }
    else if (hasOption(me, "write-ctd") && isSet(me, "write-ctd"))
    {
        if (writeCTD(me))
            return ArgumentParser::PARSE_WRITE_CTD;
        else
            return ArgumentParser::PARSE_ERROR;
    }
    else if (isSet(me, "help"))
    {
        printHelp(me, outputStream);
        return ArgumentParser::PARSE_HELP;
    }
    else if (isSet(me, "export-help"))
    {
        std::string format;
        getOptionValue(format, me, "export-help");
        printHelp(me, outputStream, format);
        return ArgumentParser::PARSE_EXPORT_HELP;
    }
    else if (argc == 1 && (me.argumentList.size() > 0 || !_allRequiredSet(me)))
    {
        // print short help and exit
        printShortHelp(me, errorStream);
        return ArgumentParser::PARSE_HELP;
    }

    // In case that everything is fine, we can now return OK.
    if (_allRequiredSet(me) && _allArgumentsSet(me))
        return ArgumentParser::PARSE_OK;

    // Otherwise, we check which options missed values.
    if (!_allRequiredSet(me))
        for (unsigned o = 0; o < length(me.optionMap); ++o)
            if (!isSet(me.optionMap[o]) && isRequired(me.optionMap[o]))
                errorStream << getAppName(me) << ": Missing value for option: " << getOptionName(me.optionMap[o]) << std::endl;
    // and arguments
    if (!_allArgumentsSet(me))
        errorStream << getAppName(me) << ": Not enough arguments were provided." << std::endl;
    errorStream << "Try '" << getAppName(me) << " --help' for more information.\n";
    return ArgumentParser::PARSE_ERROR;
}

inline ArgumentParser::ParseResult parse(ArgumentParser & me,
                                         int argc,
                                         const char * argv[])
{
    return parse(me, argc, argv, std::cout, std::cerr);
}

} // namespace seqan

#endif // SEQAN_CORE_INCLUDE_SEQAN_ARG_PARSE_ARG_PARSE_PARSE_H_