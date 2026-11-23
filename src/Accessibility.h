#pragma once

/*
This file is used to define and set accessibility related variables
*/

#include <string>

class Item{
public:
    Item(const std::string t, const std::string d, const std::string h, int fo)
        : title(t), description(d), helpText(h), focusOrder(fo) {}

    std::string title;
    std::string description;
    std::string helpText;
    unsigned int focusOrder;
};

class Slider: public Item{
public:
    Slider(const std::string t, const std::string d, const std::string h, int fo, const Item& itf)
        : Item(t,d,h,fo)
        , inputTextField(itf) {}
    Item inputTextField;
};

struct Group{
    
    Group(const std::string t, const std::string d, const std::string h, int fo)
        : title(t), description(d), helpText(h), focusOrder(fo) {}

    std::string title;
    std::string description;
    std::string helpText;
    unsigned int focusOrder;
};

namespace Focus{

    const Group bp(
        "Button Panel",
        "Contains menu controls",
        "",
        1);

    const Group ev(
        "Equation Viewer",
        "Shows the mathematical structure of the filter",
        "",
        2);

    const Group cc(
        "Coefficients Components",
        "Table that allows users to have an overview and edit filter coefficients.",
        "",
        3);

    const Group cpe(
        "Complex Plane Editor",
        "The Complex Plane Editor allows users to move and merge poles and zeros.",
        "",
        4);    


    const Group frv(
        "Frequency Response Viewer",
        "Contains magnitude and phase response plots",
        "Currently inaccessible",
        5);

    namespace buttonPanel{
        const Item undo(
            "Undo Button",
            "Revert the last change",
            "",
            1
        );

        const Item redo(
            "Redo Button",
            "Restore the last undone changes",
            "",
            2
        );

        const Item addroot(
            "Add Root Button",
            "Add a new pole or zero to the filter (see note below)",
            "",
            3
        );

        const Item exportb(
            "Export Button",
            "Export the current Filter Coefficients or Processor Chain Parameters",
            "",
            4
        );

        const Item load(
            "Load Button",
            "Load an audio file",
            "",
            5
        );

        const Item playback(
            "Playback Button",
            "Start audio playback",
            "",
            6
        );

        const Item stop(
            "Stop Button",
            "Stop audio playback",
            "",
            7
        );

        const Item loop(
            "Loop Button",
            "Repeat playback when it finishes",
            "",
            8
        );

        const Item levelText(
            "Level slider input text field",
            "Adjust the audio playback level using input text",
            "Minimum value -90 db, maximum value 6 db.",
            9
        );

        const Slider level(
            "Level slider",
            "Adjust the audio playback level",
            "",
            10,
            levelText
        );
    }

}