

In ImGui.cpp
Line 8795 (v1.90.1), Line 8569 (v1.89.x) ImGui::UpdateKeyboardInputs()
After "// Synchronize io.KeyMods and io.KeyXXX values."

    // saar, missionx workaround for copy/paste
    #ifndef LIN
      if (!io.KeyMods) // saar missionx - added if statemenet to replace the GetMergedModsFromKeys() with our own logic from custom ImgWindows class.
          io.KeyMods = GetMergedModsFromKeys();
    #else
      io.KeyMods = GetMergedModsFromKeys();
    #endif
    
    