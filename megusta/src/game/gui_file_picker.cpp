#include "gui_file_picker.h"
#include "gui.h"
#include "engine/directory_services.h"
#include <algorithm>
#include <cctype>

namespace megusta {
namespace {
bool foldedLess(const std::string& a,const std::string& b) {
    const auto less=[](unsigned char x,unsigned char y) {
        x=static_cast<unsigned char>(std::toupper(x));
        y=static_cast<unsigned char>(std::toupper(y));
        return x<y;
    };
    return std::lexicographical_compare(a.begin(),a.end(),b.begin(),b.end(),less);
}
}
GuiFilePicker::GuiFilePicker(guiElement* owner,int x,int y,int width,int height,int arrows,
                             const char* directory,const char* title)
    : puInterface(x,y),owner_(owner),list_(nullptr),scroll_(nullptr),cancel_(nullptr),
      accept_(nullptr),arrowCount_(arrows) {
    if (arrows<0) arrows=0; else if (arrows>2) arrows=2;
    refresh(directory);

    // Geometry follows puFileSelector_initialize_role @ 0046b380. PUI
    // constructors take min/max coordinates, while puSlider takes a length.
    const int sliderMinY=(arrows*5+10)*4;
    const int sliderMaxY=height-arrows*20-30;
    scroll_=new puSlider(width-30,sliderMinY,sliderMaxY-sliderMinY,TRUE);
    scroll_->setUserData(this);
    scroll_->setValue(1.0f);
    scroll_->setCallback(scrollbarChanged);
    list_=new puListBox(10,40,width-40,height-30,filePointers_.data());
    list_->setUserData(this);
    list_->setLabel(title);
    list_->setLabelPlace(0);
    list_->setStyle(-9);
    list_->setCallback(selectionChanged);
    list_->setValue(0);
    const int cancelRight=width<170 ? width/2-5 : 80;
    const int acceptLeft=width<170 ? width/2+5 : 90;
    const int acceptRight=width<170 ? width-10 : 160;
    cancel_=new puButton(10,10,cancelRight,40);
    cancel_->setLegend("Cancel"); cancel_->setUserData(this); cancel_->setCallback(cancel);
    accept_=new puButton(acceptLeft,10,acceptRight,40);
    // The adjacent recovered string at 00507d60 precedes "Cancel" and is
    // the selector's "Open" action label.
    accept_->setLegend("Open"); accept_->setUserData(this); accept_->setCallback(accept);
    if (arrows) {
        auto* up=new puArrowButton(width-30,(arrows*5+5)*4,width-10,(arrows*5+10)*4,1);
        up->setUserData(this); up->setCallback(scrollArrow);
        arrows_.push_back(up);
        auto* down=new puArrowButton(width-30,height-arrows*20-30,width-10,height-arrows*20-10,0);
        down->setUserData(this); down->setCallback(scrollArrow);
        arrows_.push_back(down);
        if (arrows==2) {
            auto* pageUp=new puArrowButton(width-30,40,width-10,60,3);
            pageUp->setUserData(this); pageUp->setCallback(scrollArrow);
            arrows_.push_back(pageUp);
            auto* pageDown=new puArrowButton(width-30,height-50,width-10,height-30,2);
            pageDown->setUserData(this); pageDown->setCallback(scrollArrow);
            arrows_.push_back(pageDown);
        }
    }
    close();
    reveal();
}
GuiFilePicker::~GuiFilePicker() {
    // PUI list widgets borrow the file pointer table. Destroy its children
    // while the backing strings and pointer table are still alive.
    empty();
}
void GuiFilePicker::setSize(int width,int height) {
    puInterface::setSize(width,height);
    if (list_) list_->setSize(width-40,height-70);
    if (scroll_) {
        scroll_->setPosition(width-30,(arrowCount_+2)*20);
        scroll_->setSize(20,height-arrowCount_*40-70);
    }
    if (cancel_) {
        cancel_->setSize(width<170 ? width/2-15 : 70,20);
    }
    if (accept_) {
        accept_->setSize(width<170 ? width/2-15 : 70,20);
        accept_->setPosition(width<170 ? width/2+5 : 90,10);
    }
    for (auto* arrow:arrows_) {
        const int type=arrow->getArrowType();
        int y=0;
        switch (type) {
        case 0: y=height-arrowCount_*20-30; break;
        case 1: y=(arrowCount_+1)*20; break;
        case 2: y=height-50; break;
        case 3: y=40; break;
        default: continue;
        }
        arrow->setPosition(width-30,y);
    }
}
void GuiFilePicker::refresh(const char* directory) {
    filenames_.clear(); filePointers_.clear();
    auto* it=EngineDirectoryIterator::open(directory);
    if (it) {
        while (char* name=it->nextFileName()) {
            if (name[0]=='.' && (!name[1] || (name[1]=='.' && !name[2]))) continue;
            filenames_.emplace_back(name);
        }
        EngineDirectoryIterator::close(it);
    }
    std::sort(filenames_.begin(),filenames_.end(),foldedLess);
    filePointers_.reserve(filenames_.size()+1);
    for (auto& name:filenames_) filePointers_.push_back(name.data());
    filePointers_.push_back(nullptr);
}
void GuiFilePicker::publishSelection() {
    if (!list_) return;
    const int index=list_->getValue();
    if (index<0 || static_cast<std::size_t>(index)>=filenames_.size()) {
        setValue(const_cast<char*>(""));
        return;
    }
    setValue(const_cast<char*>(filenames_[index].c_str()));
}
void GuiFilePicker::selectionChanged(puObject* object) {
    auto* self=static_cast<GuiFilePicker*>(object->getUserData()); if (self) self->publishSelection();
}
void GuiFilePicker::updateListFromSlider() {
    if (!list_ || !scroll_) return;
    const int top=static_cast<int>((1.0f-scroll_->getFloatValue())*list_->getNumItems());
    list_->setTopItem(top);
}
void GuiFilePicker::scrollbarChanged(puObject* object) {
    auto* self=static_cast<GuiFilePicker*>(object->getUserData());
    if (self) self->updateListFromSlider();
}
void GuiFilePicker::updateSliderFromArrow(puArrowButton* arrow) {
    if (!list_ || !scroll_) return;
    const int last=list_->getNumItems()-1;
    if (last<=0) return;
    const int kind=arrow->getArrowType();
    const int delta=kind==1 ? 1 : kind==0 ? -1 : kind==3 ? 10 : kind==2 ? -10 : 0;
    const int current=static_cast<int>((1.0f-scroll_->getFloatValue())*last+0.5f);
    const int top=std::max(0,std::min(last,current+delta));
    scroll_->setValue(static_cast<float>(1.0f-static_cast<float>(top)/static_cast<float>(last)));
    list_->setTopItem(top);
}
void GuiFilePicker::scrollArrow(puObject* object) {
    auto* self=static_cast<GuiFilePicker*>(object->getUserData());
    if (self) self->updateSliderFromArrow(static_cast<puArrowButton*>(object));
}
void GuiFilePicker::cancel(puObject* object) {
    auto* self=static_cast<GuiFilePicker*>(object->getUserData());
    if (self) {
        self->setValue(const_cast<char*>(""));
        self->invokeCallback();
    }
}
void GuiFilePicker::accept(puObject* object) {
    auto* self=static_cast<GuiFilePicker*>(object->getUserData());
    if (self) {
        self->publishSelection();
        self->invokeCallback();
    }
}
}
