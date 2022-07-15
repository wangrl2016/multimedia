//
// Created by wangrl2016 on 2022/7/15.
//

#include <memory>
#include <vector>
#include <glog/logging.h>

class Person {
public:
    Person() : age_(0) {
        LOG(INFO) << __FUNCTION__;
    }

    explicit Person(int age) : age_(age) {
        LOG(INFO) << __FUNCTION__ << ", age " << age_;
    }

    Person(const Person& other) {
        LOG(INFO) << __FUNCTION__ << " copy constructor";
        this->age_ = other.age_;
    }

    Person& operator=(const Person& other) {
        LOG(INFO) << __FUNCTION__ << "Person copy assignment operator";
        this->age_ = other.age_;
        return *this;
    }

    ~Person() {
        LOG(INFO) << __FUNCTION__;
    }

    [[nodiscard]] int age() const { return age_; }

    void set_age(int age) { age_ = age; }

private:
    int age_;
};

class Student : public Person {
public:
    Student() : number_(0) {
        LOG(INFO) << __FUNCTION__;
    }

    Student(const Student& other)  : Person(other) {
        LOG(INFO) << __FUNCTION__ << " copy constructor";
        this->number_ = other.number_;
    }

    Student& operator=(const Student& other) {
        LOG(INFO) << __FUNCTION__ << "Student copy assignment operator";
        Person::operator=(other);
        this->number_ = other.number_;
        return *this;
    }

    Student(int number, int age) : number_(number), Person(age) {
        LOG(INFO) << __FUNCTION__ << ", number " << number_;
    }

    ~Student() {
        LOG(INFO) << __FUNCTION__;
    }

    [[nodiscard]] int number() const { return number_; }

    void set_number(int number) { number_ = number; }

    void Print() {
        LOG(INFO) << "number " << number() << ", age " << age();
    }

private:
    int number_;
};

class Class {
public:
    Class() {
        LOG(INFO) << __FUNCTION__;
    }

    ~Class() {
        LOG(INFO) << __FUNCTION__;
    }

    void Init(std::vector<std::shared_ptr<Student>> students) {
        students_ = std::move(students);
    }

    size_t Size() {
        return students_.size();
    }

    void PushBack(const std::shared_ptr<Student>& student) {
        students_.push_back(student);
    }

private:
    std::vector<std::shared_ptr<Student>> students_;
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    // Initialize Google’s logging library.
    google::InitGoogleLogging(argv[0]);
    FLAGS_stderrthreshold = google::GLOG_INFO;
    {
        Student student1;
        // copy constructor
        Student student2 = student1;    // NOLINT
        Student student3;
        // copy assign operator
        student3 = student2;
        student3.Print();
    }

    {
        Student student1(15, 5);
        Student student2;
        student2.set_number(student1.number());
        student2.set_age(student1.age());
        student2.Print();
    }

    {
        Class class1;
        class1.PushBack(std::make_shared<Student>(16, 6));
        std::shared_ptr<Student> student1 = std::make_shared<Student>(17, 7);
        class1.PushBack(student1);
    }

    {
        std::vector<std::shared_ptr<Student>> class2;
        class2.push_back(std::make_shared<Student>(18, 8));
        std::vector<std::shared_ptr<Student>> class3;
        LOG(INFO) << "Size " << class2.size();
        class3 = class2;
        LOG(INFO) << "Size " << class3.size();
    }

    {
        std::vector<std::shared_ptr<Student>> class4;
        class4.push_back(std::make_shared<Student>(19, 9));
        Class class5;
        class5.Init(class4);
        LOG(INFO) << "Size " << class4.size();
        LOG(INFO) << "Size " << class5.Size();
    }

    return 0;
}
