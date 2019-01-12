#ifndef Enums_hpp
#define Enums_hpp

/** @file Enums.hpp
  * @brief 
  * @author C.D. Clark III
  * @date 01/11/19
  */

enum class AutoPilot {ON,OFF};
enum class UserInputMode {COMMAND, INSERT, PASSTHROUGH};
enum class LineStatus {EMPTY, INPROCESS, LOADED, RELOAD};
enum class OutputMode {ALL, NONE, FILTERED};

enum class CommandModeActions { None
                              , SwitchToInsertMode
                              , SwitchToPassthroughMode
                              , Quit
                              , Return
                              , ResizeWindow
                              , NextLine
                              , PrevLine
                              , TurnOffStdout
                              , TurnOnStdout
                              , ToggleStdout
                              };
enum class InsertModeActions { None
                             , SwitchToCommandMode
                             , BackOneCharacter
                             , SkipOneCharacter
                             , Return
                             };
enum class PassthroughModeActions { None
                                  , SwitchToCommandMode
                                  };



#endif // include protector
