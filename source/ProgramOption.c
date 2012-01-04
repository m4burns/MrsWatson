//
//  ProgramOption.c
//  MrsWatson
//
//  Created by Nik Reiman on 1/2/12.
//  Copyright (c) 2012 Teragon Audio. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ProgramOption.h"
#include "EventLogger.h"
#include "StringUtilities.h"

static void _addNewProgramOption(const ProgramOptions programOptions, const int index,
  const char* name, const char* help, boolean hasShortForm, ProgramOptionArgumentType argumentType) {
  ProgramOption programOption = malloc(sizeof(ProgramOptionMembers));

  programOption->index = index;
  programOption->name = newCharStringWithCapacity(STRING_LENGTH_SHORT);
  copyToCharString(programOption->name, name);
  programOption->help = newCharStringWithCapacity(STRING_LENGTH_LONG);
  copyToCharString(programOption->help, help);
  programOption->hasShortForm = hasShortForm;
  
  programOption->argumentType = argumentType;
  programOption->argument = newCharString();
  programOption->enabled = false;

  programOptions[index] = programOption;
}

ProgramOption* newProgramOptions(void) {
  ProgramOptions programOptions = malloc(sizeof(ProgramOptions) * NUM_OPTIONS);

  // TODO: Expand help for options
  _addNewProgramOption(programOptions, OPTION_BLOCKSIZE, "blocksize", "Blocksize", true, ARGUMENT_TYPE_REQUIRED);
  _addNewProgramOption(programOptions, OPTION_CHANNELS, "channels", "Number of channels", true, ARGUMENT_TYPE_REQUIRED);
  _addNewProgramOption(programOptions, OPTION_COLOR_LOGGING, "color", "Color-coded logging output", false, ARGUMENT_TYPE_OPTIONAL);
  _addNewProgramOption(programOptions, OPTION_DISPLAY_INFO, "display-info", "Print information about the plugin(s)", false, ARGUMENT_TYPE_NONE);
  _addNewProgramOption(programOptions, OPTION_HELP, "help", "Print help", true, ARGUMENT_TYPE_NONE);
  _addNewProgramOption(programOptions, OPTION_INPUT_SOURCE, "input", "Input source", true, ARGUMENT_TYPE_REQUIRED);
  _addNewProgramOption(programOptions, OPTION_PCM_FILE_NUM_CHANNELS, "pcm-file-num-channels", "Number of channels to use when reading raw PCM data", false, ARGUMENT_TYPE_REQUIRED);
  _addNewProgramOption(programOptions, OPTION_PCM_FILE_SAMPLERATE, "pcm-file-samplerate", "Sample rate to use when reading raw PCM data", false, ARGUMENT_TYPE_REQUIRED);
  _addNewProgramOption(programOptions, OPTION_PLUGIN, "plugin", "Plugin(s) to process", true, ARGUMENT_TYPE_REQUIRED);
  _addNewProgramOption(programOptions, OPTION_QUIET, "quiet", "Only log critical errors", true, ARGUMENT_TYPE_NONE);
  _addNewProgramOption(programOptions, OPTION_SAMPLERATE, "samplerate", "Set sample rate", true, ARGUMENT_TYPE_REQUIRED);
  _addNewProgramOption(programOptions, OPTION_VERBOSE, "verbose", "Verbose logging", true, ARGUMENT_TYPE_NONE);
  _addNewProgramOption(programOptions, OPTION_VERSION, "version", "Print version and copyright information", false, ARGUMENT_TYPE_NONE);

  return programOptions;
}

static boolean _isStringShortOption(const char* testString) {
  return (testString != NULL && strlen(testString) == 2 && testString[0] == '-');
}

static boolean _isStringLongOption(const char* testString) {
  return (testString != NULL && strlen(testString) > 2 && testString[0] == '-' && testString[1] == '-');  
}

static ProgramOption _findProgramOption(ProgramOptions programOptions, const char* optionString) {
  if(_isStringShortOption(optionString)) {
    for(int i = 0; i < NUM_OPTIONS; i++) {
      ProgramOption potentialMatchOption = programOptions[i];
      if(potentialMatchOption->hasShortForm && potentialMatchOption->name->data[0] == optionString[1]) {
        return potentialMatchOption;
      }
    }
  }

  if(_isStringLongOption(optionString)) {
    ProgramOption optionMatch = NULL;
    CharString optionStringWithoutDashes = newCharStringWithCapacity(STRING_LENGTH_SHORT);
    strncpy(optionStringWithoutDashes->data, optionString + 2, strlen(optionString) - 2);
    for(int i = 0; i < NUM_OPTIONS; i++) {
      ProgramOption potentialMatchOption = programOptions[i];
      if(isCharStringEqualTo(potentialMatchOption->name, optionStringWithoutDashes, false)) {
        optionMatch = potentialMatchOption;
        break;
      }
    }
    freeCharString(optionStringWithoutDashes);
    return optionMatch;
  }

  // If no option was found, then return null
  return NULL;
}

static boolean _fillOptionArgument(ProgramOption programOption, int* currentArgc, int argc, char** argv) {
  if(programOption->argumentType == ARGUMENT_TYPE_NONE) {
    return true;
  }
  else if(programOption->argumentType == ARGUMENT_TYPE_OPTIONAL) {
    int potentialNextArgc = *currentArgc + 1;
    if(potentialNextArgc >= argc) {
      return true;
    }
    else {
      char* potentialNextArg = argv[potentialNextArgc];
      // If the next string in the sequence is NOT an argument, we assume it is the optional argument
      if(!_isStringShortOption(potentialNextArg) && !_isStringLongOption(potentialNextArg)) {
        copyToCharString(programOption->argument, potentialNextArg);
        (*currentArgc)++;
        return true;
      }
      else {
        // Otherwise, it is another option, but that's ok
        return true;
      }
    }
  }
  else if(programOption->argumentType == ARGUMENT_TYPE_REQUIRED) {
    int nextArgc = *currentArgc + 1;
    if(nextArgc >= argc) {
      logCritical("Option '%s' requires an argument, but none was given", programOption->name->data);
      return false;
    }
    else {
      char* nextArg = argv[nextArgc];
      if(_isStringShortOption(nextArg) || _isStringLongOption(nextArg)) {
        logCritical("Option '%s' requires an argument, but '%s' is not valid", programOption->name->data, nextArg);
        return false;
      }
      else {
        copyToCharString(programOption->argument, nextArg);
        (*currentArgc)++;
        return true;
      }
    }
  }
  else {
    logInternalError("Unknown argument type '%d'", programOption->argumentType);
    return false;
  }
}

boolean parseCommandLine(ProgramOptions programOptions, int argc, char** argv) {
  for(int argumentIndex = 1; argumentIndex < argc; argumentIndex++) {
    const ProgramOption option = _findProgramOption(programOptions, argv[argumentIndex]);
    if(option == NULL) {
      logCritical("Invalid option '%s'", argv[argumentIndex]);
      return false;
    }
    else {
      option->enabled = true;
      if(!_fillOptionArgument(option, &argumentIndex, argc, argv)) {
        return false;
      }
    }
  }

  // If we make it to here, return true
  return true;
}

void printProgramOptions(ProgramOptions programOptions) {
  for(int i = 0; i < NUM_OPTIONS; i++) {
    printf("  ");
    ProgramOption programOption = programOptions[i];

    if(programOption->hasShortForm) {
      printf("-%c, ", programOption->name->data[0]);
    }

    // All arguments have a long form
    printf("--%s", programOption->name->data);

    switch(programOption->argumentType) {
      case ARGUMENT_TYPE_REQUIRED:
        printf(" (argument)");
        break;
      case ARGUMENT_TYPE_OPTIONAL:
        printf(" [argument]");
        break;
      case ARGUMENT_TYPE_NONE:
      default:
        break;
    }

    // Newline and indentation before help
    CharString wrappedHelpString = newCharStringWithCapacity(STRING_LENGTH_LONG);
    wrapCharStringForTerminal(programOption->help->data, wrappedHelpString->data, 4);
    printf("\n    %s\n", wrappedHelpString->data);
    freeCharString(wrappedHelpString);
  }
}

static void _freeProgramOption(ProgramOption programOption) {
  freeCharString(programOption->name);
  freeCharString(programOption->help);
  freeCharString(programOption->argument);
  free(programOption);
}

void freeProgramOptions(ProgramOptions programOptions) {
  for(int i = 0; i < NUM_OPTIONS; i++) {
    ProgramOption option = programOptions[i];
    _freeProgramOption(option);
  }
  free(programOptions);
}
