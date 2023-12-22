#include "map.h"

void Brush::CalculateAABB()
{
    min = { 1e30f, 1e30f, 1e30f};
    max = {-1e30f,-1e30f,-1e30f };

    for (Poly const& poly : this->polys)
    {
        min.Minimize(poly.min);
        max.Maximize(poly.max);
    }
}

std::vector<Poly> CSG::Union(std::vector<Brush> const& brushes)
{
    // TODO: Currently not implemented
    abort(); 

    std::vector<Poly> ret;


    /* ORIGINAL UNION CODE - CAN BE USED AS REFERENCE
    Brush* pClippedList = CopyList();
    Brush* pClip = pClippedList;
    Brush* pBrush = NULL;
    Poly* pPolyList = NULL;

    bool			bClipOnPlane = false;
    unsigned int	uiBrushes = GetNumberOfBrushes();

    for (unsigned int i = 0; i < uiBrushes; i++)
    {
        pBrush = this;
        bClipOnPlane = false;

        for (unsigned int j = 0; j < uiBrushes; j++)
        {
            if (i == j)
            {
                bClipOnPlane = true;
            }
            else
            {
                if (pClip->AABBIntersect(pBrush))
                {
                    pClip->ClipToBrush(pBrush, bClipOnPlane);
                }
            }

            pBrush = pBrush->GetNext();
        }

        pClip = pClip->GetNext();
    }

    pClip = pClippedList;

    while (pClip != NULL)
    {
        if (pClip->GetNumberOfPolys() != 0)
        {
            //
            // Extract brushes left over polygons and add them to the list
            //
            Poly* pPoly = pClip->GetPolys()->CopyList();

            if (pPolyList == NULL)
            {
                pPolyList = pPoly;
            }
            else
            {
                pPolyList->AddPoly(pPoly);
            }

            pClip = pClip->GetNext();
        }
        else
        {
            //
            // Brush has no polygons and should be deleted
            //
            if (pClip == pClippedList)
            {
                pClip = pClippedList->GetNext();

                pClippedList->SetNext(NULL);

                delete pClippedList;

                pClippedList = pClip;
            }
            else
            {
                Brush* pTemp = pClippedList;

                while (pTemp != NULL)
                {
                    if (pTemp->GetNext() == pClip)
                    {
                        break;
                    }

                    pTemp = pTemp->GetNext();
                }

                pTemp->m_pNext = pClip->GetNext();
                pClip->SetNext(NULL);

                delete pClip;

                pClip = pTemp->GetNext();
            }
        }
    }

    delete pClippedList;

    */
    return ret;
}
