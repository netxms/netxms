for name in open("nodes.txt").readlines():
    node = session.findObjectByName(name.strip())
    if node:
        comments = "New Comment"
        existingComments = node.getComments() # will return null if there are no comments
        if existingComments:
            comments += "\n" + existingComments
        session.updateObjectComments(node.getObjectId(), comments)
