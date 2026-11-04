/*
** si_s_names.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_names.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

/* 690e3850 -- freeLeafData */
void __fastcall freeLeafData(__GLcontext *gc, void **dataList)
{
    /*
    ** Note that the actual data pointed to by the elements of this list
    ** have already been freed with the callback.
    */
    ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, dataList);
}

/* 690e3860 -- freeBranch */
void __fastcall freeBranch(__GLcontext *gc, __GLnamesBranch *branch)
{
    ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, branch);
}

/* 690e3870 -- __glNamesFreeArray */
void __fastcall __glNamesFreeArray(__GLcontext *gc, __GLnamesArray *array)
{
    GLuint i;

    for (i = 0; i < array->nbranches; i++) {
        ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, array->branches[i]);
    }
    for (i = 0; i < array->nleaves; i++) {
        ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, array->leaves[i]);
    }

    __glNamesFreeTree(gc, array, array->tree, 0);

    ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, array);
}

/* 690e3bc0 -- fixMemoryProblem */
GLboolean __fastcall fixMemoryProblem(__GLcontext *gc, __GLnamesArray *array)
{
    GLuint i;

    for (i = array->nbranches; i < __GL_DL_EXTRA_BRANCHES; i++) {
        array->branches[i] = (__GLnamesBranch*)
                ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, sizeof(__GLnamesBranch));
        if (array->branches[i] == NULL) {
            array->nbranches = i;
            return GL_FALSE;
        }
    }
    array->nbranches = __GL_DL_EXTRA_BRANCHES;
    for (i = array->nleaves; i < __GL_DL_EXTRA_LEAVES; i++) { 
        array->leaves[i] = (__GLnamesLeaf*)
                ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, sizeof(__GLnamesLeaf));
        if (array->leaves[i] == NULL) {
            array->nleaves = i;
            return GL_FALSE;
        }
    }
    array->nleaves = __GL_DL_EXTRA_LEAVES;
    return GL_TRUE;
}

/* 690e3c50 -- allocLeafData */
GLboolean __fastcall allocLeafData(__GLcontext *gc, __GLnamesLeaf *leaf)
{
    GLint number;
    GLint i;

    number = leaf->end - leaf->start + 1;
    leaf->dataList = (void **) 
        ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, (size_t)(sizeof(void *)*number));
    if (!leaf->dataList) return GL_FALSE;

    for (i=0; i < number; i++) {
        leaf->dataList[i] = leaf->dataInfo->empty;
    }
    return GL_TRUE;
}

/* 690e3df0 -- addChild */
void __fastcall addChild(__GLnamesBranch *parent, __GLnamesBranch *child, 
                     GLint curdepth, GLint maxdepth)
{
    GLuint maxval;

    maxval = computeMax(child, curdepth+1, maxdepth);

    child->parent = parent;
    if (maxval > parent->medium && parent->children[1] != NULL) {
        /* This becomes the third child */
        parent->children[2] = child;

        /* Propagate the maximum value for this child to its parents */
        pushMaxVal(parent, maxval);
    } else if (maxval > parent->low) {
        /* This becomes the second child */
        parent->children[2] = parent->children[1];
        parent->children[1] = child;
        parent->medium = maxval;

        if (parent->children[2] == NULL) {
            pushMaxVal(parent, maxval);
        }
    } else {
        parent->children[2] = parent->children[1];
        parent->children[1] = parent->children[0];
        parent->children[0] = child;
        parent->medium = parent->low;
        parent->low = maxval;
    }
}

/* 690e3e70 -- computeMax */
GLuint __fastcall computeMax(__GLnamesBranch *branch, GLint curdepth, 
                         GLint maxdepth)
{
    __GLnamesLeaf *leaf;

    while (curdepth < maxdepth) {
        if (branch->children[2] != NULL) {
            branch = branch->children[2];
        } else if (branch->children[1] != NULL) {
            return branch->medium;
        } else {
            return branch->low;
        }
        curdepth++;
    }
    leaf = (__GLnamesLeaf *) branch;
    return leaf->end;
}

/* 690e3ea0 -- pushMaxVal */
void __fastcall pushMaxVal(__GLnamesBranch *child, GLuint maxval)
{
    __GLnamesBranch *parent;

    while ((parent = child->parent)) {
        if (parent->children[0] == child) {
            parent->low = maxval;
            if (parent->children[1] != NULL) {
                return;
            }
        } else if (parent->children[1] == child) {
            parent->medium = maxval;
            if (parent->children[2] != NULL) {
                return;
            }
        } else {
            assert(parent->children[2] == child);
        }
        child = parent;
    }
}

/* 690e3ed0 -- splitParent */
void __fastcall splitParent(__GLnamesBranch *parent, 
                                __GLnamesBranch *newParent, 
                                __GLnamesBranch *extraChild, 
                                GLint curdepth, 
                                GLint maxdepth)
{
    __GLnamesBranch *children[4], *tempchild;
    GLuint maxvals[4], tempval;
    int i;

    /* Collect our four children */
    children[0] = parent->children[0];
    maxvals[0] = parent->low;
    children[1] = parent->children[1];
    maxvals[1] = parent->medium;
    children[2] = parent->children[2];
    maxvals[2] = computeMax(children[2], curdepth+1, maxdepth);
    children[3] = extraChild;
    maxvals[3] = computeMax(extraChild, curdepth+1, maxdepth);

    /* Children 0-2 are sorted.  Sort child 3 too. */
    for (i = 3; i > 0; i--) {
        if (maxvals[i] < maxvals[i-1]) {
            tempval = maxvals[i];
            tempchild = children[i];
            maxvals[i] = maxvals[i-1];
            children[i] = children[i-1];
            maxvals[i-1] = tempval;
            children[i-1] = tempchild;
        }
    }

    /* Construct the two parents */
    parent->low = maxvals[0];
    parent->children[0] = children[0];
    parent->medium = maxvals[1];
    parent->children[1] = children[1];
    parent->children[2] = NULL;
    children[0]->parent = parent;
    children[1]->parent = parent;
    pushMaxVal(parent, maxvals[1]);

    newParent->low = maxvals[2];
    newParent->children[0] = children[2];
    newParent->medium = maxvals[3];
    newParent->children[1] = children[3];
    newParent->children[2] = NULL;
    children[2]->parent = newParent;
    children[3]->parent = newParent;
}

/* 690e3fc0 -- buildParent */
void __fastcall buildParent(__GLnamesBranch *parent, __GLnamesBranch *child1, 
                        __GLnamesBranch *child2, GLint depth)
{
    GLuint maxChild1, maxChild2;

    child1->parent = parent;
    child2->parent = parent;
    maxChild1 = computeMax(child1, 0, depth);
    maxChild2 = computeMax(child2, 0, depth);
    if (maxChild2 > maxChild1) {
        parent->children[0] = child1;
        parent->low = maxChild1;
        parent->children[1] = child2;
        parent->medium = maxChild2;
    } else {
        parent->children[0] = child2;
        parent->low = maxChild2;
        parent->children[1] = child1;
        parent->medium = maxChild1;
    }
}

/* 690e42c0 -- deleteChild */
void __fastcall deleteChild(__GLnamesArray *array, __GLnamesBranch *parent, 
                        __GLnamesBranch *child, GLint depth)
{
    GLuint maxval;
    GLint maxdepth;

    maxdepth = array->depth;

    if (parent->children[0] == child) {
        parent->children[0] = parent->children[1];
        parent->children[1] = parent->children[2];
        parent->children[2] = NULL;
        parent->low = parent->medium;
        if (parent->children[1] != NULL) {
            maxval = computeMax(parent->children[1], depth+1, maxdepth);
            parent->medium = maxval;
        } else parent->medium = 0;
    } else if (parent->children[1] == child) {
        parent->children[1] = parent->children[2];
        parent->children[2] = NULL;
        if (parent->children[1] != NULL) {
            maxval = computeMax(parent->children[1], depth+1, maxdepth);
            parent->medium = maxval;
        } else parent->medium = 0;
    } else {
        assert(parent->children[2] == child);
        parent->children[2] = NULL;
        pushMaxVal(parent, parent->medium);
    }
}

/* 690e4b90 -- __glNamesUnlockData */
void __fastcall __glNamesUnlockData(__GLcontext *gc, void *data)
{
    GLint *pRefcount;
    assert(data);

    pRefcount = data;
    (*pRefcount)--;                /* decrement the refcount */
    assert(*pRefcount >= 0);         /* less than zero references?*/
    if (*pRefcount == 0) {
        /*
        ** We are the last person to see this list alive.  Free it.
        */
       ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, data);
    }
}

/* 690e4ba0 -- __glNamesUnlockDataList */
void __fastcall __glNamesUnlockDataList(__GLcontext *gc, GLsizei n, 
                             void *dataList[])
{
    GLint i;
    GLint *pRefcount;

    /*
    ** The refcount comes first in all data definitions, so the
    ** data pointer also points to the refcount.
    */
    for (i = 0; i < n; i++) {
        pRefcount = (GLint *)(dataList[i]);
        (*pRefcount) --;                        /* decrement the refcount */
        assert(*pRefcount >= 0);         /* less than zero references?*/
        if (*pRefcount == 0) {
            /*
            ** We are the last person to see this list alive.  Free it.
            */
            ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, (void *)pRefcount);
        }
    }
}

/* 690e4fa0 -- copyLeafInfo */
void __fastcall copyLeafInfo(__GLnamesLeaf *leaf, __GLnamesLeaf *newleaf)
{
    GLint offset;
    GLuint number;
    GLuint i;

    number = newleaf->end - newleaf->start + 1;
    offset = newleaf->start - leaf->start;

    for (i = 0; i < number; i++) {
        newleaf->dataList[i] = leaf->dataList[i+offset];
    }
}

/* 690e3780 -- __glNamesFreeTree */
void __fastcall __glNamesFreeTree(__GLcontext *gc, __GLnamesArray *array,
                       __GLnamesBranch *tree, GLint depth)
{
    GLuint i;
    __GLnamesLeaf *leaf;
    void *empty;
    GLint maxdepth = array->depth;

    if (tree == NULL) return;

    if (depth < maxdepth) {
        __glNamesFreeTree(gc, array, tree->children[2], depth+1);
        __glNamesFreeTree(gc, array, tree->children[1], depth+1);
        __glNamesFreeTree(gc, array, tree->children[0], depth+1);

        freeBranch(gc, tree);
    } else {
        leaf = (__GLnamesLeaf *) tree;
        empty = array->dataInfo->empty;

        if (leaf->dataList) {
            for (i=leaf->start; i<=leaf->end; i++) {
                if (leaf->dataList[i - leaf->start] != empty) {
                    assert(leaf->dataInfo->free!=NULL);
                    (*leaf->dataInfo->free)(gc, 
                                leaf->dataList[i - leaf->start]);
                    leaf->dataList[i - leaf->start] = empty;
                }
            }
        }
        freeLeaf(gc, leaf);
    }
}

/* 690e3830 -- freeLeaf */
void __fastcall freeLeaf(__GLcontext *gc, __GLnamesLeaf *leaf)
{
    if (leaf->dataList) {
        freeLeafData(gc, leaf->dataList);
    }
    ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, leaf);
}

/* 690e38d0 -- __glNamesNewData */
GLboolean __fastcall __glNamesNewData(__GLcontext *gc, __GLnamesArray *array, 
                                GLuint name, void *data)
{
    __GLnamesLeaf *leaf, *newleaf;
    GLint entry;
    GLuint start, end;

    leaf = findLeaf(array, name, GL_TRUE);

    /*
    ** First we check for possible memory problems, since it will be 
    ** difficult to back out once we start.
    */
    if (leaf == NULL || leaf->dataList == NULL) {
        /* 
        ** May need memory in these cases.
        */
        if (array->nbranches != __GL_DL_EXTRA_BRANCHES ||
                array->nleaves != __GL_DL_EXTRA_LEAVES) {
            if (!fixMemoryProblem(gc, array)) {
                __glSetError(GL_OUT_OF_MEMORY);
                return GL_FALSE;
            }
        }
    }

    if (!leaf) {
        /*
        ** Make new leaf with just this display list 
        */
        leaf = allocLeaf(gc, array);
        leaf->start = leaf->end = name;
        if (data) {
            if (!allocLeafData(gc, leaf)) {
                /* 
                ** Bummer.  No new list for you! 
                */
                freeLeaf(gc, leaf);
                __glSetError(GL_OUT_OF_MEMORY);
                return GL_FALSE;
            }
            leaf->dataList[0] = data;
            (*(GLint *)data) = 1;                /* set the refcount */
        }
        insertLeaf(gc, array, leaf);
        mergeLeaf(gc, array, leaf);
        return GL_TRUE;
    } else if (leaf->dataList) {
        /*
        ** Simply update the appropriate entry in the lists array 
        */
        entry = name - leaf->start;
        if (leaf->dataList[entry] != leaf->dataInfo->empty) {
            assert(leaf->dataInfo->free!=NULL);
            (*leaf->dataInfo->free)(gc, leaf->dataList[entry]);
            leaf->dataList[entry] = leaf->dataInfo->empty;
        }
        if (data) {
            leaf->dataList[entry] = data;
            (*(GLint *)data) = 1;                /* set the refcount */
        }
        return GL_TRUE;
    } else {
        if (!data) {
            /*
            ** If there isn't really any list, we are done.
            */
            return GL_TRUE;
        }

        /*
        ** Allocate some or all of the lists in leaf.  If only some, then
        ** leaf needs to be split into two or three leaves.
        **
        ** First we decide what range of numbers to allocate an array for.
        ** (be careful of possible word wrap error)
        */
        start = name - __GL_DLIST_MIN_ARRAY_BLOCK/2;
        if (start < leaf->start || start > name) {
            start = leaf->start;
        }
        end = start + __GL_DLIST_MIN_ARRAY_BLOCK - 1;
        if (end > leaf->end || end < start) {
            end = leaf->end;
        }

        if (start - leaf->start < __GL_DLIST_MIN_ARRAY_BLOCK) {
            start = leaf->start;
        }
        if (leaf->end - end < __GL_DLIST_MIN_ARRAY_BLOCK) {
            end = leaf->end;
        }

        if (start == leaf->start) {
            if (end == leaf->end) {
                /* 
                ** Simply allocate the entire array.
                */
                if (!allocLeafData(gc, leaf)) {
                    /*
                    ** Whoa!  No memory!  Never mind!
                    */
                    __glSetError(GL_OUT_OF_MEMORY);
                    return GL_FALSE;
                }
                {
                    GLint entry = name - leaf->start;
                    leaf->dataList[entry] = data;
                    (*(GLint *)data) = 1;                /* set the refcount */
                }
                mergeLeaf(gc, array, leaf);
                return GL_TRUE;
            } else {
                /* 
                ** Shrink the existing leaf, and create a new one to hold
                ** the new arrays (done outside the "if" statement).
                */
                resizeLeaf(gc, leaf, end+1, leaf->end);
            }
        } else if (end == leaf->end) {
            /* 
            ** Shrink the existing leaf, and create a new one to hold
            ** the new arrays (done outside the "if" statement).
            */
            resizeLeaf(gc, leaf, leaf->start, start-1);
        } else {
            /* 
            ** Crud.  The middle of the leaf was deleted.  This is tough.
            */
            newleaf = allocLeaf(gc, array);

            newleaf->start = end+1;
            newleaf->end = leaf->end;
            resizeLeaf(gc, leaf, leaf->start, start-1);
            insertLeaf(gc, array, newleaf);
        }
        leaf = allocLeaf(gc, array);
        leaf->start = start;
        leaf->end = end;
        if (!allocLeafData(gc, leaf)) {
            /*
            ** Whoa!  No memory!  Never mind!
            */
            insertLeaf(gc, array, leaf);
            mergeLeaf(gc, array, leaf);
            __glSetError(GL_OUT_OF_MEMORY);
            return GL_FALSE;
        }
        {
            GLint entry = name - leaf->start;
            leaf->dataList[entry] = data;
            (*(GLint *)data) = 1;                /* set the refcount */
        }
        insertLeaf(gc, array, leaf);
        mergeLeaf(gc, array, leaf);
        return GL_TRUE;
    }
}

/* 690e3cc0 -- insertLeaf */
void __fastcall insertLeaf(__GLcontext *gc, __GLnamesArray *array,
                                __GLnamesLeaf *leaf)
{
    __GLnamesBranch *extraChild;
    __GLnamesBranch *branch;
    __GLnamesBranch *parent;
    __GLnamesBranch *newParent;
    GLint maxdepth, curdepth;
    GLuint number;

    number = leaf->end;
    maxdepth = array->depth;
    branch = array->tree;
    if (!branch) {
        /* No tree!  Make a one leaf tree. */
        array->depth = 0;
        array->tree = (__GLnamesBranch *) leaf;
        return;
    }

    curdepth = 0;
    while (curdepth < maxdepth) {
        if (number <= branch->low) {
            branch = branch->children[0];
        } else if (number <= branch->medium) {
            branch = branch->children[1];
        } else {
            if (branch->children[2] != NULL) {
                branch = branch->children[2];
            } else {
                branch = branch->children[1];
            }
        }
        curdepth++;
    }

    /*
    ** Ok, we just managed to work our way to the bottom of the tree.
    ** 'leaf' becomes the extraChild, and we now try to insert it anywhere
    ** it will fit.
    */
    extraChild = (__GLnamesBranch *) leaf;
    parent = branch->parent;

    curdepth--;
    while (parent) {
        if (parent->children[2] == NULL) {
            /* We have room to squeeze this node in here! */
            addChild(parent, extraChild, curdepth, maxdepth);
            return;
        }

        /*
        ** We have one parent and four children.  This simply 
        ** won't do.  We create a new parent, and end up with two 
        ** parents with two children each.  That works.
        */
        newParent = allocBranch(gc, array);
        splitParent(parent, newParent, extraChild, curdepth, maxdepth);

        /*
        ** Great.  Now newParent becomes the orphan, and we try to 
        ** trivially insert it up a level.
        */
        extraChild = newParent;
        branch = parent;
        parent = branch->parent;
        curdepth--;
    }

    /* We just reached the top node, and there is no parent, and we 
    ** still haven't managed to rid ourselves of an extra child.  So,
    ** we make a new parent to take branch and extraChild as it's two
    ** children.  We have to increase the depth of the tree, of course.
    */
    assert(curdepth == -1);
    parent = allocBranch(gc, array);
    buildParent(parent, branch, extraChild, maxdepth);
    array->tree = parent;
    array->depth++;
}

/* 690e4020 -- resizeLeaf */
void __fastcall resizeLeaf(__GLcontext *gc, __GLnamesLeaf *leaf, 
                                GLuint newstart, GLuint newend)
{
    GLuint oldstart, oldend;
    GLuint newsize, offset, i;

    oldstart = leaf->start;
    oldend = leaf->end;

    leaf->start = newstart;
    if (newend != oldend) {
        leaf->end = newend;
        pushMaxVal((__GLnamesBranch *) leaf, newend);
    }
    if (leaf->dataList == NULL) return;

    /*
    ** Copy the appropriate pointers to the begining of the array, and 
    ** realloc it.
    */
    offset = newstart - oldstart;
    newsize = newend - newstart + 1;
    if (offset) {
        for (i=0; i<newsize; i++) {
            /*
            ** Copy the whole structure with one line.
            */
            leaf->dataList[i] = leaf->dataList[i+offset];
        }
    }
    reallocLeafData(gc, leaf);
}

/* 690e4090 -- reallocLeafData */
GLboolean __fastcall reallocLeafData(__GLcontext *gc, __GLnamesLeaf *leaf)
{
    size_t number;
    void **answer;

    number = (size_t) (leaf->end - leaf->start + 1);
    answer = (void **) ((void *(__fastcall *)(__GLcontext *, void *, size_t))gc->imports[2])(gc, 
                leaf->dataList, sizeof(void *)*number);
    if (answer) {
        leaf->dataList = answer;
        return GL_TRUE;
    } else {
        /*
        ** Crud!  Out of memory!
        */
        return GL_FALSE;
    }
}

/* 690e40c0 -- mergeLeaf */
void __fastcall mergeLeaf(__GLcontext *gc, __GLnamesArray *array,
                                __GLnamesLeaf *leaf)
{
    __GLnamesLeaf *next, *prev;

    next = nextLeaf(leaf);
    if (next) {
        /* Try to merge with next leaf */
        if (leaf->end + 1 == next->start) {
            if ((leaf->dataList == NULL && next->dataList == NULL) || 
                    (next->dataList && leaf->dataList &&
                    next->end - leaf->start < __GL_DLIST_MAX_ARRAY_BLOCK)) {
                /* It's legal to merge these leaves */
                deleteLeaf(gc, array, next);
                if (!mergeLeaves(gc, leaf, next)) {
                    /* 
                    ** Ack!  No memory?  We bail on the merge.  
                    */
                    insertLeaf(gc, array, next);
                    return;
                }
            }
        }
    }

    prev = prevLeaf(leaf);
    if (prev) {
        /* Try to merge with prev leaf */
        if (prev->end + 1 == leaf->start) {
            if ((prev->dataList == NULL && leaf->dataList == NULL) || 
                    (leaf->dataList && prev->dataList &&
                    leaf->end - prev->start < __GL_DLIST_MAX_ARRAY_BLOCK)) {
                /* It's legal to merge these leaves */
                deleteLeaf(gc, array, prev);
                if (!mergeLeaves(gc, leaf, prev)) {
                    /* 
                    ** Ack!  No memory?  We bail on the merge.  
                    */
                    insertLeaf(gc, array, prev);
                    return;
                }
            }
        }
    }
}

/* 690e4190 -- deleteLeaf */
void __fastcall deleteLeaf(__GLcontext *gc, __GLnamesArray *array,
                                __GLnamesLeaf *leaf)
{
    __GLnamesBranch *orphan;
    __GLnamesBranch *parent, *newParent;
    __GLnamesBranch *grandparent;
    GLint depth, maxdepth;
    GLuint maxval;

    maxdepth = depth = array->depth;
    parent = leaf->parent;
    if (parent == NULL) {
        /* Ack!  We just nuked the only node! */
        array->tree = NULL;
        return;
    }

    deleteChild(array, parent, (__GLnamesBranch *) leaf, depth-1);

    /*
    ** depth is the depth of the child in this case.
    */
    depth--;                        
    while (parent->children[1] == NULL) {
        /* Crud.  Need to do work. */
        orphan = parent->children[0];

        /* Ax the parent, insert child into grandparent. */
        grandparent = parent->parent;

        if (grandparent == NULL) {
            /*
            ** Hmmm.  Parent was the root.  Nuke it and make the orphan
            ** the new root.
            */
            freeBranch(gc, parent);
            array->tree = orphan;
            orphan->parent = NULL;
            array->depth--;
            return;
        }

        deleteChild(array, grandparent, parent, depth-1);
        freeBranch(gc, parent);

        /* The parent is dead.  Find a new parent. */
        maxval = computeMax(orphan, depth+1, maxdepth);
        if (grandparent->children[1] == NULL || 
                maxval <= grandparent->low) {
            parent = grandparent->children[0];
        } else {
            parent = grandparent->children[1];
        }

        /* Insert orphan into new parent. */
        if (parent->children[2] != NULL) {
            newParent = allocBranch(gc, array);
            splitParent(parent, newParent, orphan, depth, maxdepth);
            /* We know there is room! */
            addChild(grandparent, newParent, depth-1, maxdepth);
            return;
        }

        /* The parent has room for the child */
        addChild(parent, orphan, depth, maxdepth);

        depth--;
        parent = grandparent;
    }
}

/* 690e43d0 -- mergeLeaves */
GLboolean __fastcall mergeLeaves(__GLcontext *gc, __GLnamesLeaf *leaf1, 
                             __GLnamesLeaf *leaf2)
{
    GLuint end;
    GLuint i;
    GLuint number, offset;

    /* If we don't have to merge lists, it is easy. */
    if (leaf1->dataList == NULL) {
        assert(leaf2->dataList == NULL);
        if (leaf1->start < leaf2->start) {
            leaf1->end = leaf2->end;
            pushMaxVal((__GLnamesBranch *) leaf1, leaf1->end);
        } else {
            leaf1->start = leaf2->start;
        }
        freeLeaf(gc, leaf2);
        return GL_TRUE;
    }

    /* 
    ** Yick!  Need to merge lists.
    */
    assert(leaf2->dataList != NULL);
    if (leaf1->start < leaf2->start) {
        /*
        ** Expand size of leaf1's array, copy leaf2's array into it,
        ** free leaf2.
        */
        offset = leaf1->end - leaf1->start + 1;
        number = leaf2->end - leaf2->start + 1;
        end = leaf1->end;
        leaf1->end = leaf2->end;
        if (!reallocLeafData(gc, leaf1)) {
            /*
            ** Heavens!  No memory?  That sucks!
            ** We won't bother merging.  It is never an absolutely critical
            ** operation.
            */
            leaf1->end = end;
            return GL_FALSE;
        }
        for (i = 0; i < number; i++) {
            leaf1->dataList[i+offset] = leaf2->dataList[i];
        }

        freeLeaf(gc, leaf2);

        pushMaxVal((__GLnamesBranch *) leaf1, leaf1->end);
    } else {
        /*
        ** Expand the size of leaf2's array, copy leaf1's array into it.
        ** Then free leaf1's array, copy leaf2's array to leaf1, and free
        ** leaf2.
        */
        offset = leaf2->end - leaf2->start + 1;
        number = leaf1->end - leaf1->start + 1;
        end = leaf2->end;
        leaf2->end = leaf1->end;
        if (!reallocLeafData(gc, leaf2)) {
            /*
            ** Heavens!  No memory?  That sucks!
            ** We won't bother merging.  It is never an absolutely critical
            ** operation.
            */
            leaf2->end = end;
            return GL_FALSE;
        }
        for (i = 0; i < number; i++) {
            leaf2->dataList[i+offset] = leaf1->dataList[i];
        }

        freeLeafData(gc, leaf1->dataList);
        leaf1->start = leaf2->start;

        leaf1->dataList = leaf2->dataList;
        leaf2->dataList = NULL;
        freeLeaf(gc, leaf2);
    }
    return GL_TRUE;
}

/* 690e4580 -- __glNamesLockDataList */
void __fastcall __glNamesLockDataList(__GLcontext *gc, __GLnamesArray *array, 
                        GLsizei n, GLenum type, GLuint base,
                        const GLvoid *names, void *dataPtrs[])
{
    __GLnamesLeaf *leaf;
    void **data;
    void *tempData;
    void *empty;
    GLuint curName;

    empty = array->dataInfo->empty;

    data = dataPtrs;

    /*
    ** Note that this code is designed to take advantage of coherence.
    ** After looking up (and locking) a single display list in 
    ** listnums[], the next list is checked for in the same leaf that
    ** contained the previous.  This will make typical uses of CallLists()
    ** quite fast (text, for example).
    */

    /*
    ** Lock access to array.
    */
    switch(type) {
      case GL_BYTE:
        /*
        ** Coded poorly for optimization purposes
        */
        {
            const GLbyte *p = (const GLbyte *) names;

Bstart:
            if (--n >= 0) {
                /* Optimization for possibly common font case */
                curName = base + *p++;
Bfind:
                leaf = findLeaf(array, curName, GL_TRUE);
                if (leaf && leaf->dataList) {
                    GLuint reldiff;
                    GLuint relend;
                    void **leafData;

                    leafData = leaf->dataList;
                    tempData = leafData[curName - leaf->start];

                    /* All possible display lists can be found here */
                    reldiff = base - leaf->start;
                    relend = leaf->end - leaf->start;

Bsave:
                    (*(GLint *)tempData)++;        /* increment the refcount */
                    *data++ = tempData;
                    if (--n >= 0) {
                        curName = *p++ + reldiff;
                        if (curName <= relend) {
                            tempData = leafData[curName];
                            goto Bsave;
                        }
                        curName = curName + leaf->start;
                        goto Bfind;
                    }
                } else {
                    (*(GLint *)empty)++;                /* increment refcount */
                    *data++ = empty;
                    goto Bstart;
                }
            }
        }
        break;
      case GL_UNSIGNED_BYTE:
        /*
        ** Coded poorly for optimization purposes
        */
        {
            const GLubyte *p = (const GLubyte *) names;

UBstart:
            if (--n >= 0) {
                /* Optimization for possibly common font case */
                curName = base + *p++;
UBfind:
                leaf = findLeaf(array, curName, GL_TRUE);
                if (leaf && leaf->dataList) {
                    GLuint reldiff;
                    GLuint relend;
                    void **leafData;

                    leafData = leaf->dataList;
                    tempData = leafData[curName - leaf->start];

                    /* All possible display lists can be found here */
                    reldiff = base - leaf->start;
                    relend = leaf->end - leaf->start;

UBsave:
                    (*(GLint *)tempData)++;        /* increment the refcount */
                    *data++ = tempData;
                    if (--n >= 0) {
                        curName = *p++ + reldiff;
                        if (curName <= relend) {
                            tempData = leafData[curName];
                            goto UBsave;
                        }
                        curName = curName + leaf->start;
                        goto UBfind;
                    }
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                    goto UBstart;
                }
            }
        }
        break;
      case GL_SHORT:
        {
            const GLshort *p = (const GLshort *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + *p++;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;        /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      case GL_UNSIGNED_SHORT:
        {
            const GLushort *p = (const GLushort *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + *p++;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;        /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      case GL_INT:
        {
            const GLint *p = (const GLint *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + *p++;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;        /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      case GL_UNSIGNED_INT:
        {
            const GLuint *p = (const GLuint *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + *p++;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;        /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      case GL_FLOAT:
        {
            const GLfloat *p = (const GLfloat *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + *p++;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;        /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      case GL_2_BYTES:
        {
            const GLubyte *p = (const GLubyte *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + ((p[0] << 8) | p[1]);
                p += 2;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;        /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      case GL_3_BYTES:
        {
            const GLubyte *p = (const GLubyte *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + ((p[0] << 16) | (p[1] << 8) | p[2]);
                p += 3;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;        /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      case GL_4_BYTES:
        {
            const GLubyte *p = (const GLubyte *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + ((p[0] << 24) | (p[1] << 16) | 
                        (p[2] << 8) | p[3]);
                p += 4;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;        /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      default:
        /* This should be impossible */
        assert(0);
    }
}

/* 690e4bd0 -- __glNamesGenRange */
GLuint __fastcall __glNamesGenRange(__GLcontext *gc, __GLnamesArray *array, 
                         GLsizei range)
{
    GLuint lastUsed;
    GLuint nextUsed;
    GLuint maxUsed;
    __GLnamesLeaf *leaf;
    __GLnamesLeaf *nextleaf;
    __GLnamesLeaf *newleaf;

    /*
    ** First we check for possible memory problems, since it will be 
    ** difficult to back out once we start.
    */
    if (array->nbranches != __GL_DL_EXTRA_BRANCHES ||
            array->nleaves != __GL_DL_EXTRA_LEAVES) {
        if (!fixMemoryProblem(gc, array)) {
            __glSetError(GL_OUT_OF_MEMORY);
            return 0;
        }
    }

    leaf = firstLeaf(array);

    /*
    ** Can we possibly allocate the appropriate number before the first leaf?
    */
    if (leaf && leaf->start > (GLuint) range) {
        if (leaf->dataList == NULL) {
            /* 
            ** Ha!  We can trivially extend leaf! 
            */
            leaf->start -= range;
            return leaf->start;
        } else {
            /* 
            ** Must make a new leaf 
            */
            newleaf = allocLeaf(gc, array);

            newleaf->start = 1;
            newleaf->end = range;
            insertLeaf(gc, array, newleaf);

            return 1;
        }
    }

    while (leaf) {
        nextleaf = nextLeaf(leaf);
        if (!nextleaf) break;

           lastUsed = leaf->end + 1;
        nextUsed = nextleaf->start;

        /* Room for (lastUsed) - (nextUsed-1) here */
        if (nextUsed - lastUsed >= (GLuint) range) {
            if (leaf->dataList == NULL) {
                /* Trivial to expand 'leaf' */
                leaf->end += range;
                pushMaxVal((__GLnamesBranch *) leaf, leaf->end);

                if (nextUsed - lastUsed == (GLuint) range && nextleaf->dataList == NULL) {
                    mergeLeaf(gc, array, leaf);
                }

                return lastUsed;
            } else if (nextleaf->dataList == NULL) {
                /* Trivial to expand 'nextleaf' */
                nextleaf->start -= range;

                return nextleaf->start;
            } else {
                newleaf = allocLeaf(gc, array);

                newleaf->start = lastUsed;
                newleaf->end = lastUsed + range - 1;
                insertLeaf(gc, array, newleaf);

                return lastUsed;
            }
        }

        leaf = nextleaf;
    }

    if (leaf == NULL) {
        newleaf = allocLeaf(gc, array);

        newleaf->start = 1;
        newleaf->end = range;
        insertLeaf(gc, array, newleaf);

        return 1;
    } else {
        lastUsed = leaf->end;
        maxUsed = lastUsed + range;
        if (maxUsed < lastUsed) {
            /* Word wrap!  Ack! */
            return 0;
        }
        if (leaf->dataList == NULL) {
            /* Trivial to expand 'leaf' */
            leaf->end += range;
            pushMaxVal((__GLnamesBranch *) leaf, leaf->end);

            return lastUsed + 1;
        } else {
            /* Need to make new leaf */
            newleaf = allocLeaf(gc, array);

            newleaf->start = lastUsed + 1;
            newleaf->end = maxUsed;
            insertLeaf(gc, array, newleaf);

            return lastUsed + 1;
        }
    }
}

/* 690e4fe0 -- __glNamesIsName */
GLboolean __fastcall __glNamesIsName(__GLcontext *gc, __GLnamesArray *array, 
                          GLuint name)
{
    /*
    ** If the name retrieves a leaf, it is in the current name space.
    */
    if (findLeaf(array, name, GL_TRUE)) {
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/* 690e5000 -- __glNamesGenNames */
void __fastcall __glNamesGenNames(__GLcontext *gc, __GLnamesArray *array,
                       GLsizei n, GLuint* names)
{
    GLuint start, nameVal;
    int i;

    if (NULL == names) return;
    start = __glNamesGenRange(gc, array, n);
    for (i=0, nameVal=start; i < n; i++, nameVal++) {
        names[i] = nameVal;
    }
    
}

/* 690e3b60 -- findLeaf */
__GLnamesLeaf * __fastcall findLeaf(__GLnamesArray *array, GLuint number, 
                                                       GLint exact)
{
    __GLnamesBranch *branch;
    __GLnamesLeaf *leaf;
    int depth = array->depth, r;

    branch = array->tree;

    while (depth > 0 && branch) {

        /* rather than following if-then-else code
         * for correct branch, evaluate all conditions
         * quickly to compute correct branch.
         */
        int r = (number > branch->low) + (number > branch->medium);
        assert(branch->low <= branch->medium);
        branch = branch->children[r];
        --depth;
    }
    if (!(leaf = (__GLnamesLeaf *) branch)) return NULL;

    /* the case we want to optimize is the one in which we
     * actually find the node, so evaluate both conditions
     * quickly, since both results are required in this case
     * and return appropriately.  the choice of the final
     * if construct is to match the current vagaries of the
     * 3.19 compiler code generator (db)
     */
    r = (leaf->end < number) | (exact&(number<leaf->start));
    if (!r) return leaf;
        return NULL;
}

/* 690e3c90 -- allocLeaf */
__GLnamesLeaf * __fastcall allocLeaf(__GLcontext *gc, __GLnamesArray *array)
{
    __GLnamesLeaf *leaf;

    leaf = (__GLnamesLeaf *) 
            ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, sizeof(__GLnamesLeaf));

    if (leaf == NULL) {
        /*
        ** Ouch!  No memory?  We had better use one of the preallocated 
        ** leaves.
        */
        assert(array->nleaves != 0);
        array->nleaves--;
        leaf = array->leaves[array->nleaves];
    }

    leaf->parent = NULL;
    leaf->dataList = NULL;
    leaf->dataInfo = array->dataInfo;

    return leaf;
}

/* 690e3dc0 -- allocBranch */
__GLnamesBranch * __fastcall allocBranch(__GLcontext *gc, __GLnamesArray *array)
{
    __GLnamesBranch *branch;

    branch = (__GLnamesBranch *) 
            ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, sizeof(__GLnamesBranch));

    if (branch == NULL) {
        /*
        ** Ouch!  No memory?  We had better use one of the preallocated 
        ** branches.
        */

        assert(array->nbranches != 0);
        array->nbranches--;
        branch = array->branches[array->nbranches];
    }

    branch->children[0] = branch->children[1] = branch->children[2] = NULL;
    branch->parent = NULL;

    return branch;
}

/* 690e4330 -- prevLeaf */
__GLnamesLeaf * __fastcall prevLeaf(__GLnamesLeaf *leaf)
{
    __GLnamesBranch *branch, *child;
    GLint reldepth;

    branch = leaf->parent;
    if (!branch) return NULL;                /* A one leaf tree! */

    child = (__GLnamesBranch *) leaf;

    /* We start off at a relative depth of 1 above the child (-1) */
    reldepth = -1;

    while (branch) {
        /* If the child was the 3rd child, branch down to the second. */
        if (branch->children[2] == child) {
            branch = branch->children[1];
            reldepth++;                /* One level lower */
            break;
        } else if (branch->children[1] == child) {
            /* If the child was the 2nd child, branch down to the first */
            branch = branch->children[0];
            reldepth++;                /* One level lower */
            break;
        } else {
            /* Must have been 1st child */
            assert(branch->children[0] == child);
        }
        /*
        ** Otherwise, we have already visited all of this branch's children,
        ** so we go up a level.
        */
        child = branch;
        branch = branch->parent;
        reldepth--;        /* One level higher */
    }
    if (!branch) return NULL;        /* All leaves visited! */

    /* Go down the 'right'most trail of this branch until we get to 
    ** a child, then return it.
    */
    while (reldepth) {
        if (branch->children[2] != NULL) {
            branch = branch->children[2];
        } else if (branch->children[1] != NULL) {
            branch = branch->children[1];
        } else {
            branch = branch->children[0];
        }
        reldepth++;                /* One level lower */
    }

    return (__GLnamesLeaf *) branch;
}

/* 690e4390 -- nextLeaf */
__GLnamesLeaf * __fastcall nextLeaf(__GLnamesLeaf *leaf)
{
    __GLnamesBranch *branch, *child;
    GLint reldepth;

    branch = leaf->parent;
    if (!branch) return NULL;                /* A one leaf tree! */

    child = (__GLnamesBranch *) leaf;

    /* We start off at a relative depth of 1 above the child (-1) */
    reldepth = -1;

    while (branch) {
        /* If the child was the 1st child, branch down to the second. */
        if (branch->children[0] == child) {
            branch = branch->children[1];
            reldepth++;                /* One level lower */
            break;
        } else if (branch->children[1] == child) {
            /* 
            ** If the child was the 2nd child, and there is a third, branch
            ** down to it.  
            */
            if (branch->children[2] != NULL) {
                branch = branch->children[2];
                reldepth++;        /* One level lower */
                break;
            }
        } else {
            /* Must have been 3rd child */
            assert(branch->children[2] == child);
        }
        /*
        ** Otherwise, we have already visited all of this branch's children,
        ** so we go up a level.
        */
        child = branch;
        branch = branch->parent;
        reldepth--;        /* One level higher */
    }
    if (!branch) return NULL;        /* All leaves visited! */

    /* Go down the 'left'most trail of this branch until we get to 
    ** a child, then return it.
    */
    while (reldepth) {
        branch = branch->children[0];
        reldepth++;                /* One level lower */
    }

    return (__GLnamesLeaf *) branch;
}

/* 690e4540 -- __glNamesLockData */
void * __fastcall __glNamesLockData(__GLcontext *gc, __GLnamesArray *array, 
                        GLuint name)
{
    __GLnamesLeaf *leaf;
    void *data;
    GLint offset;

    /*
    ** Lock access to data.
    */
    leaf = findLeaf(array, name, GL_TRUE);
    if (leaf == NULL || leaf->dataList == NULL) {
        return NULL;
    }
    offset = name - leaf->start;
    data = leaf->dataList[offset];
    if (data) {
        (*(GLint *)data)++;                /* Increment the refcount. */
    }
    return data;
}

/* 690e4da0 -- firstLeaf */
__GLnamesLeaf * __fastcall firstLeaf(__GLnamesArray *array)
{
    __GLnamesBranch *branch;
    GLint maxdepth, curdepth;

    maxdepth = array->depth;
    curdepth = 0;
    branch = array->tree;

    /* No tree, no leaves! */
    if (!branch) return NULL;

    /* Take the 'left'most branch until we reach a leaf */
    while (curdepth != maxdepth) {
        branch = branch->children[0];
        curdepth++;
    }
    return (__GLnamesLeaf *) branch;
}

/* 690e5030 -- __glNamesDeleteNames */
void __fastcall __glNamesDeleteNames(__GLcontext *gc, __GLnamesArray *array,
                          GLsizei n, const GLuint* names)
{
    GLuint start, rangeVal;
    GLint i;

    /*
    ** Because of resizing leaves, etc, it is best to work in ranges
    ** as much as possible.  So break the list into ranges
    ** and delete them that way.  This degrades into deleting
    ** them one at a time if the list is disjoint or non-ascending.
    ** It also only calls DeleteRange once if the list is a
    ** contiguous range of names.
    */
    start = rangeVal = names[0];
    for (i=0; i < n; i++, rangeVal++) {
        if (names[i] != rangeVal) {
            __glNamesDeleteRange(gc,array,start,rangeVal-start);
            start = rangeVal = names[i];
        }
    }
    __glNamesDeleteRange(gc,array,start,rangeVal-start);
    return;
}
