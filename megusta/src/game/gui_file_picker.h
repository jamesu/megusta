// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "pu.h"
#include <vector>
#include <string>

namespace megusta {
class guiElement;
// Recovered game layout used for guiElement.FILEPICKER (type 9).
class GuiFilePicker final : public puInterface {
public:
    GuiFilePicker(guiElement*,int x,int y,int width,int height,int arrows,
                  const char* directory,const char* title);
    ~GuiFilePicker() override;
    void setSize(int width,int height) override;
    char** files() { return filePointers_.data(); }
    void refresh(const char* directory);
private:
    static void selectionChanged(puObject*);
    static void scrollbarChanged(puObject*);
    static void scrollArrow(puObject*);
    static void cancel(puObject*);
    static void accept(puObject*);
    void publishSelection();
    void updateListFromSlider();
    void updateSliderFromArrow(puArrowButton*);
    guiElement* owner_;
    std::vector<std::string> filenames_;
    std::vector<char*> filePointers_;
    puListBox* list_;
    puSlider* scroll_;
    puButton* cancel_;
    puButton* accept_;
    std::vector<puArrowButton*> arrows_;
    int arrowCount_;
};
}
