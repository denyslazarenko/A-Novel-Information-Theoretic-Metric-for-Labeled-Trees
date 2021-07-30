def create_random_tree(n_leaves):
    random.seed(0)
    np.random.seed(0)

    root = Tree()
    next_deq = deque([root])
    cur_leaves_num = 1

    while cur_leaves_num < n_leaves:
        if n_leaves - cur_leaves_num <= 2:
            node_num_of_leaves = 2
        else:
            node_num_of_leaves = np.random.choice(range(2, n_leaves - cur_leaves_num))
        # take node from being counted where I add leaves
        cur_leaves_num += node_num_of_leaves - 1
        if random.randint(0, 1):
            current_node = next_deq.pop()
        else:
            current_node = next_deq.popleft()
        new_leaves = []
        for i in range(node_num_of_leaves):
            c = current_node.add_child()
            new_leaves.append(c)
        next_deq.extend(new_leaves)

    for i, n in enumerate(root.get_leaves()):
        n.name = i
    root = name_nodes(root, n_leaves)
    return root
