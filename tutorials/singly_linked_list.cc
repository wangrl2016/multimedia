//
// Created by wangrl2016 on 2022/7/14.
//

#include <iostream>

// Definition for singly linked list.
struct ListNode {
    int val;
    ListNode* next;

    ListNode() : val(0), next(nullptr) {}

    explicit ListNode(int x) : val(x), next(nullptr) {}

    ListNode(int x, ListNode* next) : val(x), next(next) {}
};

ListNode* MergeTwoLists(ListNode* list1, ListNode* list2) { // NOLINT
    if (list1 == nullptr)
        return list2;
    if (list2 == nullptr)
        return list1;

    if (list1->val < list2->val) {
        list1->next = MergeTwoLists(list1->next, list2);
        return list1;
    } else {
        list2->next = MergeTwoLists(list1, list2->next);
        return list2;
    }
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    // list1 1 3 4
    auto* list1 = new ListNode(1);
    list1->next = new ListNode(3, new ListNode(4));
    // list2 2 3 5
    auto* list2 = new ListNode(2);
    list2->next = new ListNode(3, new ListNode(5));

    auto* list = MergeTwoLists(list1, list2);
    while (list) {
        std::cout << list->val << std::endl;
        list = list->next;
    }

    return 0;
}
