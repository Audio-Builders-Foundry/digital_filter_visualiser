#pragma once

/*
This file is used to define and set accessibility related variables
*/

#include <string>

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

}