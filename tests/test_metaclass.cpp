/**
 * @file test_metaclass.cpp
 * @brief Unit tests for the MetaClass system
 */

#include <gtest/gtest.h>
#include "MetaClass.h"
#include <memory>

using namespace backtrader::meta;

class MetaClassTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures
    }
    
    void TearDown() override {
        // Clean up
    }
};

// Test ParameterCollection basic functionality
TEST_F(MetaClassTest, ParameterCollection_BasicOperations) {
    ParameterCollection params;
    
    // Add parameters
    params.addParameter(ParameterDef("period", 15, "Moving average period"));
    params.addParameter(ParameterDef("alpha", 0.5, "Alpha coefficient"));
    params.addParameter(ParameterDef("name", std::string("test"), "Component name"));
    
    // Test parameter existence
    EXPECT_TRUE(params.hasParameter("period"));
    EXPECT_TRUE(params.hasParameter("alpha"));
    EXPECT_TRUE(params.hasParameter("name"));
    EXPECT_FALSE(params.hasParameter("nonexistent"));
    
    // Test default values
    EXPECT_EQ(params.getValue<int>("period"), 15);
    EXPECT_DOUBLE_EQ(params.getValue<double>("alpha"), 0.5);
    EXPECT_EQ(params.getValue<std::string>("name"), "test");
    
    // Test isDefault
    EXPECT_TRUE(params.isDefault("period"));
    EXPECT_TRUE(params.isDefault("alpha"));
    EXPECT_TRUE(params.isDefault("name"));
    
    // Set values
    params.setValue("period", 20);
    params.setValue("alpha", 0.8);
    params.setValue("name", std::string("modified"));
    
    // Test modified values
    EXPECT_EQ(params.getValue<int>("period"), 20);
    EXPECT_DOUBLE_EQ(params.getValue<double>("alpha"), 0.8);
    EXPECT_EQ(params.getValue<std::string>("name"), "modified");
    
    // Test isDefault after modification
    EXPECT_FALSE(params.isDefault("period"));
    EXPECT_FALSE(params.isDefault("alpha"));
    EXPECT_FALSE(params.isDefault("name"));
}

// Test ParameterCollection with default fallback
TEST_F(MetaClassTest, ParameterCollection_DefaultFallback) {
    ParameterCollection params;
    
    // Add parameter with default
    params.addParameter(ParameterDef("threshold", 100.0, "Threshold value"));
    
    // Test getValue with default fallback
    EXPECT_DOUBLE_EQ(params.getValue("threshold", 50.0), 100.0);  // Should return defined default
    EXPECT_DOUBLE_EQ(params.getValue("nonexistent", 50.0), 50.0);  // Should return fallback
}

// Test ParameterCollection update functionality
TEST_F(MetaClassTest, ParameterCollection_Update) {
    ParameterCollection params1, params2;
    
    // Set up first collection
    params1.addParameter(ParameterDef("period", 15, "Period"));
    params1.addParameter(ParameterDef("alpha", 0.5, "Alpha"));
    
    // Set up second collection
    params2.addParameter(ParameterDef("period", 20, "Period modified"));  // Override existing
    params2.addParameter(ParameterDef("beta", 0.3, "Beta"));  // Add new
    
    // Update params1 with params2
    params1.update(params2);
    
    // Check merged results
    EXPECT_TRUE(params1.hasParameter("period"));
    EXPECT_TRUE(params1.hasParameter("alpha"));
    EXPECT_TRUE(params1.hasParameter("beta"));
    
    EXPECT_EQ(params1.getValue<int>("period"), 20);  // Should be updated
    EXPECT_DOUBLE_EQ(params1.getValue<double>("alpha"), 0.5);  // Should remain
    EXPECT_DOUBLE_EQ(params1.getValue<double>("beta"), 0.3);  // Should be added
}

// Test AutoInfoClass basic functionality
TEST_F(MetaClassTest, AutoInfoClass_BasicOperations) {
    auto info = std::make_unique<AutoInfoClass>();
    
    // Note: AutoInfoClass starts empty, so we need to add parameters
    // This tests the parameter system indirectly
    
    // Test empty state
    EXPECT_EQ(info->getKeys().size(), 0);
    EXPECT_EQ(info->getValues().size(), 0);
    EXPECT_EQ(info->getItems().size(), 0);
    
    // Validation should pass for empty params
    EXPECT_TRUE(info->validateParams());
}

// Test MetaBase lifecycle
TEST_F(MetaClassTest, MetaBase_Lifecycle) {
    class TestMetaBase : public MetaBase {
    public:
        std::vector<LifecycleStage> executed_stages;
        
        void doPreNew() override {
            executed_stages.push_back(LifecycleStage::PreNew);
        }
        
        void doNew() override {
            executed_stages.push_back(LifecycleStage::New);
        }
        
        void doPreInit() override {
            executed_stages.push_back(LifecycleStage::PreInit);
        }
        
        void doInit() override {
            executed_stages.push_back(LifecycleStage::Init);
        }
        
        void doPostInit() override {
            executed_stages.push_back(LifecycleStage::PostInit);
        }
        
        void onStageComplete(LifecycleStage stage) override {
            // Verify stages are executed in correct order
            switch (stage) {
                case LifecycleStage::PreNew:
                    EXPECT_EQ(executed_stages.size(), 1);
                    break;
                case LifecycleStage::New:
                    EXPECT_EQ(executed_stages.size(), 2);
                    break;
                case LifecycleStage::PreInit:
                    EXPECT_EQ(executed_stages.size(), 3);
                    break;
                case LifecycleStage::Init:
                    EXPECT_EQ(executed_stages.size(), 4);
                    break;
                case LifecycleStage::PostInit:
                    EXPECT_EQ(executed_stages.size(), 5);
                    break;
            }
        }
    };
    
    TestMetaBase meta;
    meta.executeLifecycle();
    
    // Verify all stages were executed in correct order
    EXPECT_EQ(meta.executed_stages.size(), 5);
    EXPECT_EQ(meta.executed_stages[0], LifecycleStage::PreNew);
    EXPECT_EQ(meta.executed_stages[1], LifecycleStage::New);
    EXPECT_EQ(meta.executed_stages[2], LifecycleStage::PreInit);
    EXPECT_EQ(meta.executed_stages[3], LifecycleStage::Init);
    EXPECT_EQ(meta.executed_stages[4], LifecycleStage::PostInit);
}

// Test MetaParams template
TEST_F(MetaClassTest, MetaParams_BasicOperations) {
    class TestClass : public MetaParams<TestClass> {
    public:
        void setupParams() {
            DEFINE_PARAMETER(int, period, 15, "Period parameter");
            DEFINE_PARAMETER(double, factor, 1.0, "Factor parameter");
            DEFINE_PARAMETER(std::string, mode, "default", "Mode parameter");
        }
    protected:
        void initializeParams() override {
            setupParams();
        }
    };
    
    TestClass test_obj;
    test_obj.initialize();  // Proper two-phase initialization
    
    // Test parameter access
    EXPECT_EQ(test_obj.getParam<int>("period"), 15);
    EXPECT_DOUBLE_EQ(test_obj.getParam<double>("factor"), 1.0);
    EXPECT_EQ(test_obj.getParam<std::string>("mode"), "default");
    
    // Test parameter shortcuts (p method)
    EXPECT_EQ(test_obj.p<int>("period"), 15);
    EXPECT_DOUBLE_EQ(test_obj.p<double>("factor"), 1.0);
    EXPECT_EQ(test_obj.p<std::string>("mode"), "default");
    
    // Test parameter modification
    test_obj.setParam("period", 30);
    test_obj.setP("factor", 2.0);
    
    EXPECT_EQ(test_obj.getParam<int>("period"), 30);
    EXPECT_DOUBLE_EQ(test_obj.getParam<double>("factor"), 2.0);
    
    // Test default state
    EXPECT_FALSE(test_obj.isDefault("period"));
    EXPECT_FALSE(test_obj.isDefault("factor"));
    EXPECT_TRUE(test_obj.isDefault("mode"));
}

// Test ItemCollection
TEST_F(MetaClassTest, ItemCollection_Operations) {
    ItemCollection<int> collection;
    
    // Test empty state
    EXPECT_EQ(collection.size(), 0);
    EXPECT_TRUE(collection.empty());
    
    // Add items
    collection.append(10, "first");
    collection.append(20, "second");
    collection.append(30);  // No name
    
    // Test size
    EXPECT_EQ(collection.size(), 3);
    EXPECT_FALSE(collection.empty());
    
    // Test index access
    EXPECT_EQ(collection[0], 10);
    EXPECT_EQ(collection[1], 20);
    EXPECT_EQ(collection[2], 30);
    
    // Test name access
    EXPECT_EQ(collection.getByName("first"), 10);
    EXPECT_EQ(collection.getByName("second"), 20);
    
    // Test names
    auto names = collection.getNames();
    EXPECT_EQ(names.size(), 2);  // Only named items
    EXPECT_EQ(names[0], "first");
    EXPECT_EQ(names[1], "second");
    
    // Test modification through reference
    collection[0] = 100;
    collection.getByName("second") = 200;
    
    EXPECT_EQ(collection[0], 100);
    EXPECT_EQ(collection[1], 200);
    EXPECT_EQ(collection.getByName("first"), 100);
    EXPECT_EQ(collection.getByName("second"), 200);
    
    // Test iterator
    int sum = 0;
    for (const auto& item : collection) {
        sum += item;
    }
    EXPECT_EQ(sum, 330);  // 100 + 200 + 30
    
    // Test clear
    collection.clear();
    EXPECT_EQ(collection.size(), 0);
    EXPECT_TRUE(collection.empty());
}

// Test PackageInfo
TEST_F(MetaClassTest, PackageInfo_Creation) {
    // Test simple package
    PackageInfo pkg1("numpy");
    EXPECT_EQ(pkg1.package_name, "numpy");
    EXPECT_EQ(pkg1.alias, "numpy");
    EXPECT_TRUE(pkg1.from_imports.empty());
    
    // Test package with alias
    PackageInfo pkg2("numpy", "np");
    EXPECT_EQ(pkg2.package_name, "numpy");
    EXPECT_EQ(pkg2.alias, "np");
    EXPECT_TRUE(pkg2.from_imports.empty());
    
    // Test package with from imports
    PackageInfo pkg3("math", {"sin", "cos", "tan"});
    EXPECT_EQ(pkg3.package_name, "math");
    EXPECT_EQ(pkg3.alias, "math");
    EXPECT_EQ(pkg3.from_imports.size(), 3);
    EXPECT_EQ(pkg3.from_imports[0], "sin");
    EXPECT_EQ(pkg3.from_imports[1], "cos");
    EXPECT_EQ(pkg3.from_imports[2], "tan");
}

// Test parameter validation
TEST_F(MetaClassTest, ParameterCollection_Validation) {
    ParameterCollection params;
    
    // Add required and optional parameters
    params.addParameter(ParameterDef("required_param", 0, "Required parameter", true));
    params.addParameter(ParameterDef("optional_param", 100, "Optional parameter", false));
    
    // Should fail validation initially (required param has default value)
    EXPECT_FALSE(params.validate());
    
    // Set required parameter to non-default value
    params.setValue("required_param", 42);
    
    // Should pass validation now
    EXPECT_TRUE(params.validate());
    
    // Optional parameter doesn't affect validation
    params.setValue("optional_param", 200);
    EXPECT_TRUE(params.validate());
}

// Test error handling
TEST_F(MetaClassTest, ErrorHandling) {
    ParameterCollection params;
    params.addParameter(ParameterDef("test_param", 42, "Test parameter"));
    
    // Test accessing non-existent parameter
    EXPECT_THROW(params.getValue<int>("nonexistent"), std::runtime_error);
    
    // Test type mismatch
    EXPECT_THROW(params.getValue<std::string>("test_param"), std::runtime_error);
    
    // Test setting value for undefined parameter
    EXPECT_THROW(params.setValue("undefined", 123), std::runtime_error);
    
    // ItemCollection error handling
    ItemCollection<int> collection;
    collection.append(10, "test");
    
    // Test out of range access
    EXPECT_THROW(collection[1], std::out_of_range);
    
    // Test accessing non-existent name
    EXPECT_THROW(collection.getByName("nonexistent"), std::runtime_error);
}

// Integration test
TEST_F(MetaClassTest, Integration_CompleteWorkflow) {
    class CompleteTestClass : public MetaParams<CompleteTestClass> {
    private:
        std::string lifecycle_log_;
        
    protected:
        void initializeParams() override {
            DEFINE_PARAMETER(int, window, 20, "Window size");
            DEFINE_PARAMETER(double, threshold, 0.95, "Threshold value");
            DEFINE_PARAMETER(std::string, algorithm, "sma", "Algorithm type");
            DEFINE_REQUIRED_PARAMETER(std::string, data_source, "Data source");
        }
        
    public:
        void doPreNew() override {
            lifecycle_log_ += "PreNew;";
            MetaParams::doPreNew();
        }
        
        void doNew() override {
            lifecycle_log_ += "New;";
            MetaParams::doNew();
        }
        
        void doPreInit() override {
            lifecycle_log_ += "PreInit;";
            MetaParams::doPreInit();
        }
        
        void doInit() override {
            lifecycle_log_ += "Init;";
            MetaParams::doInit();
        }
        
        void doPostInit() override {
            lifecycle_log_ += "PostInit;";
            MetaParams::doPostInit();
        }
        
        std::string getLifecycleLog() const { return lifecycle_log_; }
    };
    
    CompleteTestClass obj;
    obj.initialize();  // Proper two-phase initialization
    
    // Test initial parameter values
    EXPECT_EQ(obj.p<int>("window"), 20);
    EXPECT_DOUBLE_EQ(obj.p<double>("threshold"), 0.95);
    EXPECT_EQ(obj.p<std::string>("algorithm"), "sma");
    
    // Set required parameter
    obj.setP<std::string>("data_source", "test_data.csv");
    
    // Validation should pass now
    EXPECT_TRUE(obj.validateParams());
    
    // Test lifecycle execution (already executed during initialize())
    EXPECT_EQ(obj.getLifecycleLog(), "PreNew;New;PreInit;Init;PostInit;");
    
    // Test parameter modification and state checking
    obj.setP("window", 50);
    EXPECT_FALSE(obj.isDefault("window"));
    EXPECT_TRUE(obj.notDefault("window"));
    EXPECT_TRUE(obj.isDefault("threshold"));
    EXPECT_FALSE(obj.notDefault("threshold"));
}