#include <iostream>
#include <random>
#include <fstream>

#define DISTANCE_TEST_HEADER_FILENAME "root_distance_tests.h"
#define ROOTORDER_TEST_HEADER_FILENAME "root_order_tests.h"

float get_random_float()
{
    static std::default_random_engine e;
    static std::uniform_real_distribution<> dis(-1, 1); // range [-1, 1)
    return dis(e);
}

int get_num_tests(int order, int ymin, int ymax, int x0, float k)
{
    /*
    // Reverse of the Sigmoid function shifted horizontaly:
    // f(x) = ymin + (ymax - ymin) / (1 + e^(k(x-x0)),
    // where :
    //      [ymin,ymax] is the range of examples,
    //      x0 is the function has declined by 50%
    //      k defines the steepness of the declining curve
    // Example:
    // So for ymax=10, ymin=3, x0=20 and k=0.3, the function becomes:
    // f(x) = 3 + 7 / (1 + e^0.3(x-20))
    // ref:check f\left(x\right)=3\ +\ \frac{7}{\left(1+e^{0.3\left(x-20\right)}\right)} in https://www.desmos.com/calculator
    */
    return static_cast<int>(std::round( ymin + (ymax-ymin) / (1 + std::exp( k * static_cast<float>(order-x0)))));
}

void generate_root_distance_tests(int argc, char* argv[])
{
    std::string filename(DISTANCE_TEST_HEADER_FILENAME);
    if (argc==2){
        filename = std::string(argv[1]) + "/" + filename;
    }

    std::ofstream outStream(filename);

    std::string rootTypes[2] = {"pole", "zero"};
    std::string dts[2] = {"real","complex"};

    for (int typeidx=0; typeidx<2;++typeidx)
    {   
        if (typeidx == 0)
            outStream << "#define COEFFICIENTS_TO_ROOTS_DISTANCE_TEST_POLES_XLIST\\"<<'\n';
        else    
            outStream << "#define COEFFICIENTS_TO_ROOTS_DISTANCE_TEST_ZEROS_XLIST\\"<<'\n';

        // for each test
        for (int order=1;order<=32;++order)
        {
            int orderCtr=0;
            int num_tests = get_num_tests(order, 3, 10, 20, .3);

            for (int sample=0; sample<num_tests; sample++ )
            {

                std::string dt = dts[(order+sample)%2];
            
                std::string testLine = "X(\"distance} ";
                testLine+= std::to_string(order);
                int norder = (rootTypes[typeidx].compare("pole")) ? order : -order;
                testLine+= (": "+dt+" "+rootTypes[typeidx]+" order "+std::to_string(++orderCtr) +"\", { {" + std::to_string(norder) +","); 

                float x,y;
                if (dt.compare("complex")){
                    y = 0.0f;
                    x= get_random_float();
                }
                else{
                    y = get_random_float();
                    x= get_random_float();
                }
                testLine += (std::to_string(x) + "," + std::to_string(y) + "} } )\\");
                outStream << '\t' << testLine << '\n';
            }
        }
        outStream<<"\n\n";
    }
    outStream.close();
}

void partition_rootOrder(int n, std::vector<std::vector<int> >& partitions)
{
    /*
    Modified the printAllUniqueParts function drawn from www.sanfoundry.com 
    Reference : https://www.sanfoundry.com/cpp-program-generate-all-unique-partitions-integer/ 
    */

    int p[n]; // An array to store a partition
    int k = 0;  // Index of last element in a partition
    p[k] = n;  // Initialize first partition as number itself
    int i = 0; // Index of current partition

    // This loop first prints the current partition then generates the next partition. 
    // The loop stops when the current partition has all 1s
    while (true)
    {
        // store new partition
        partitions.push_back(std::vector<int>(p, p + k+1));
 
        // Find the rightmost non-one value in p[]. Also, update the rem_val
        // So that we know how much value can be accommodated
 
        int rem_val = 0;
        while (k >= 0 && p[k] == 1)
        {
            rem_val += p[k];
            k--;
        }
 
        // if k < 0, all the values are 1 so there are no more partitions
 
        if (k < 0)  
            return;
 
        // Decrease the p[k] found above and adjust the rem_val
        p[k]--;
        rem_val++;
 
        // If rem_val is more, then the sorted order is violeted.  
        // Divide rem_val in differnt values of size p[k] 
        // Copy these values at different positions after p[k]
        while (rem_val > p[k])
        {
            p[k+1] = p[k];
            rem_val = rem_val - p[k];
            k++;
        }

        // Copy rem_val to next position and increment position
        p[k+1] = rem_val;
        k++;
    }
}

void generate_root_order_tests(int argc, char* argv[])
{
    std::string filename(ROOTORDER_TEST_HEADER_FILENAME);
    if (argc==2){
        filename = std::string(argv[1]) + "/" + filename;
    }

    std::ofstream outStream(filename);
    outStream << "#define COEFFICIENTS_TO_ROOTS_TEST_XLIST\\"<<'\n';

    std::string dimtypes[2] = {"real","complex"};

    // for each order
    for (int order=1;order<=32;++order)
    {
        int orderCtr=0;
        int num_tests = get_num_tests(order, 3, 20, 17, 0.7);
        std::vector<std::vector<int> > partitions;
        partition_rootOrder(order, partitions);
        float ratio = static_cast<float>(partitions.size()) / num_tests;

        // for each test sample
        for (int sample=0; sample<num_tests; sample++ )
        {
            int index = static_cast<int>(sample * ratio);
            
            // std::cout<<"order "<<order<<" sample "<<sample<<"/"<<num_tests<<" index "<<index<<"/"<<partitions.size()<<std::endl;
            size_t numRoots = partitions[index].size();

            std::string testLine = "X(\"" + std::to_string(order) + "("+std::to_string(++orderCtr)+"):Pole order config (";

            std::string values = "{";
            
            // for each sub root
            for (size_t i=0; i< numRoots; i++)
            {    
                int subRootOrder = partitions[index][i];
                
                std::string dt = dimtypes[(order+i)%2];
                
                // edit config text
                testLine+= (std::to_string(subRootOrder)+" "+dt);

                // edit root value text
                float x,y;
                if (dt.compare("complex")){
                    y = 0.0f;
                    x= get_random_float();
                }
                else{
                    y = get_random_float();
                    x= get_random_float();
                }
                values += "{" + std::to_string(-subRootOrder) + "," + (std::to_string(x) + "," + std::to_string(y) + "}");

                if (i!=numRoots-1) 
                {
                    testLine+=",";
                    values+=",";
                }
            }

            testLine += ")\"," + values + "} )\\";
            outStream << '\t' << testLine << '\n';
        }
    }
    outStream.close();
}

int main(int argc, char* argv[]){

    // seed for generating deterministic random values
    srand (static_cast <unsigned> (time(0)));

    generate_root_distance_tests(argc, argv);

    generate_root_order_tests(argc, argv);

}