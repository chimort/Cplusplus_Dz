#include <vector>

class Statement {  
public:  
    virtual std::vector<int> apply(std::vector<int> in) const = 0;  

    Statement() = default;  
    Statement(unsigned arguments, unsigned results, bool pure): arguments(arguments), results(results), pure(pure) {}  

    virtual ~Statement() = default;  

    bool is_pure() const {  
        return pure;  
    }  

    unsigned get_arguments_count() const {  
        return arguments;  
    }  

    unsigned get_results_count() const {  
        return results;  
    }  

protected:  
    unsigned arguments;  
    unsigned results;  
    bool pure;  
};