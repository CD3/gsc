#ifndef Enums_hpp
#define Enums_hpp

/** @file Enums.hpp
  * @brief 
  * @author C.D. Clark III
  * @date 01/11/19
  */

enum class UserInputMode {COMMAND, INSERT, PASSTHROUGH, AUTO };
enum class LineStatus {EMPTY, INPROCESS, LOADED, RELOAD};
enum class OutputMode {ALL, NONE, FILTERED};
enum class AutoPilotMode { SEMI, FULL };

enum class CommandModeActions {
                                SwitchToInsertMode
                              , SwitchToPassthroughMode
                              , SwitchToAutoMode
                              , Quit
                              , Return
                              , ResizeWindow
                              , NextLine
                              , PrevLine
                              , TurnOffStdout
                              , TurnOnStdout
                              , ToggleStdout
                              , None
                              };
enum class InsertModeActions {
                               SwitchToCommandMode
                             , BackOneCharacter
                             , SkipOneCharacter
                             , Return
                             , Disabled
                             , None
                             };
enum class PassthroughModeActions {
                                    SwitchToCommandMode
                                  , None
                                  };

enum class AutoModeActions {
                             SwitchToCommandMode
                           , SwitchToFullAuto
                           , SwitchToSemiAuto
                           , Return
                           , None
                           };



#endif // include protector
