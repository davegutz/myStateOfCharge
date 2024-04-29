import glob
import os
import re

from reportlab.lib import utils
from reportlab.pdfgen import canvas


def convert(text):
    return int(text) if text.isdigit() else text


def alphanum_key(key):
    return [convert(c) for c in re.split('([0-9]+)', key)]


def sorted_nicely(_l):
    """
    # http://stackoverflow.com/questions/2669059/how-to-sort-alpha-numeric-set-in-python

    Sort the given iterable in the way that humans expect.
    """
    return sorted(_l, key=alphanum_key)


def cleanup_fig_files(fig_files):
    # Clean up after itself.   Other fig files already in root will get plotted by unite_pictures_into_pdf
    # Cleanup other figures in root folder by hand
    for fig_file in fig_files:
        try:
            os.remove(fig_file)
        except OSError:
            pass


def precleanup_fig_files(output_pdf_name='unite_pictures.pdf', path_to_pdfs='.'):
    # Clean up before itself.   Other fig files already in root will get plotted by unite_pictures_into_pdf
    # Cleanup other figures in root folder by hand
    from glob import glob
    from os import remove
    for file in glob(os.path.join(path_to_pdfs, output_pdf_name+'*.pdf')):
        print("removing", file)
        try:
            remove(file)
        except OSError:
            pass


# ----------------------------------------------------------------------
def unite_pictures_into_pdf(outputPdfName='unite_pictures.pdf', save_pdf_path='.', pathToPictures='.', splitType="picture",
                            numberOfEntitiesInOnePdf=9, listWithImagesExtensions=None,
                            picturesAreInRootFolder=True, nameOfPart="picture"):
    # print(f"unite_pictures_into_pdf:\n{outputPdfName=}\n{save_pdf_path=}\n{pathToPictures=}\n{splitType=}\n{numberOfEntitiesInOnePdf=}\n{listWithImagesExtensions=}\n{picturesAreInRootFolder=}\n{nameOfPart=}")

    if listWithImagesExtensions is None:
        listWithImagesExtensions = ["png", "jpg"]

    if numberOfEntitiesInOnePdf < 1:
        print("Wrong value of numberOfEntitiesInOnePdf.")
        return

    if len(listWithImagesExtensions) == 0:
        print("listWithImagesExtensions is empty.")
        return

    if picturesAreInRootFolder is True:
        foldersInsideFolderWithPictures = sorted_nicely(glob.glob(os.path.join(pathToPictures, '*')))
        if len(foldersInsideFolderWithPictures) != 0:
            picturesPathsForEachFolder = []
            for iFolder in foldersInsideFolderWithPictures:
                picturePathsInFolder = []
                for jExtension in listWithImagesExtensions:
                    picturePathsInFolder.extend(glob.glob(iFolder + "*." + jExtension))
                picturesPathsForEachFolder.append(sorted_nicely(picturePathsInFolder))
            if splitType == "folder":
                numberOfFoldersAdded = 0
                for iFolder in picturesPathsForEachFolder:
                    if (numberOfFoldersAdded % numberOfEntitiesInOnePdf) == 0:
                        endNumber = numberOfFoldersAdded + numberOfEntitiesInOnePdf
                        if endNumber > len(picturesPathsForEachFolder):
                            endNumber = len(picturesPathsForEachFolder)
                        filename = []
                        if numberOfEntitiesInOnePdf > 1:
                            filename = os.path.join(save_pdf_path, outputPdfName + "_" + nameOfPart + "_" + str(
                                numberOfFoldersAdded + 1) + '-' + str(endNumber) + "_of_" + str(
                                len(picturesPathsForEachFolder)) + ".pdf")
                        elif numberOfEntitiesInOnePdf == 1:
                            filename = os.path.join(save_pdf_path, outputPdfName + "_" + nameOfPart + "_" + str(
                                numberOfFoldersAdded + 1) + "_of_" + str(len(picturesPathsForEachFolder)) + ".pdf")
                        c = canvas.Canvas(filename)
                    for jPicture in iFolder:
                        img = utils.ImageReader(jPicture)
                        imagesize = img.getSize()
                        # noinspection PyUnboundLocalVariable
                        c.setPageSize(imagesize)
                        c.drawImage(jPicture, 0, 0)
                        c.showPage()
                    numberOfFoldersAdded += 1
                    if (numberOfFoldersAdded % numberOfEntitiesInOnePdf) == 0:
                        c.save()
                        # noinspection PyUnboundLocalVariable
                        print("created", filename)
                if (numberOfFoldersAdded % numberOfEntitiesInOnePdf) != 0:
                    c.save()
                    print("created", filename)
            elif splitType == "picture":
                numberOfPicturesAdded = 0
                totalNumberOfPictures = 0
                for iFolder in picturesPathsForEachFolder:
                    totalNumberOfPictures += len(iFolder)
                for iFolder in picturesPathsForEachFolder:
                    for jPicture in iFolder:
                        if (numberOfPicturesAdded % numberOfEntitiesInOnePdf) == 0:
                            endNumber = numberOfPicturesAdded + numberOfEntitiesInOnePdf
                            if endNumber > totalNumberOfPictures:
                                endNumber = totalNumberOfPictures
                            filename = []
                            if numberOfEntitiesInOnePdf > 1:
                                filename = os.path.join(save_pdf_path, outputPdfName + "_" + nameOfPart + "_" + str(
                                    numberOfPicturesAdded + 1) + '-' + str(endNumber) + "_of_" + str(
                                    totalNumberOfPictures) + ".pdf")
                            elif numberOfEntitiesInOnePdf == 1:
                                filename = os.path.join(save_pdf_path, outputPdfName + "_" + nameOfPart + "_" + str(
                                    numberOfPicturesAdded + 1) + "_of_" + str(totalNumberOfPictures) + ".pdf")
                            c = canvas.Canvas(filename)
                        img = utils.ImageReader(jPicture)
                        imagesize = img.getSize()
                        # noinspection PyUnboundLocalVariable
                        c.setPageSize(imagesize)
                        c.drawImage(jPicture, 0, 0)
                        c.showPage()
                        numberOfPicturesAdded += 1
                        if (numberOfPicturesAdded % numberOfEntitiesInOnePdf) == 0:
                            c.save()
                            # noinspection PyUnboundLocalVariable
                            print("created", filename)
                if (numberOfPicturesAdded % numberOfEntitiesInOnePdf) != 0:
                    c.save()
                    print("created", filename)
            elif splitType == "none":
                filename = os.path.join(save_pdf_path, outputPdfName + ".pdf")
                c = canvas.Canvas(filename)
                for iFolder in picturesPathsForEachFolder:
                    for jPicture in iFolder:
                        img = utils.ImageReader(jPicture)
                        imagesize = img.getSize()
                        c.setPageSize(imagesize)
                        c.drawImage(jPicture, 0, 0)
                        c.showPage()
                c.save()
                print("created", filename)
            else:
                print("Wrong splitType value")
        else:
            print("No pictures found.")
        return

    if picturesAreInRootFolder:
        picturesInsideFolderWithPictures = []
        for iExtension in listWithImagesExtensions:
            picturesInsideFolderWithPictures.extend(glob.glob(pathToPictures + "\\*." + iExtension))
        picturesInsideFolderWithPictures = sorted_nicely(picturesInsideFolderWithPictures)
        if len(picturesInsideFolderWithPictures) != 0:
            if splitType == "picture":
                numberOfPicturesAdded = 0
                totalNumberOfPictures = len(picturesInsideFolderWithPictures)
                for iPicture in picturesInsideFolderWithPictures:
                    if (numberOfPicturesAdded % numberOfEntitiesInOnePdf) == 0:
                        endNumber = numberOfPicturesAdded + numberOfEntitiesInOnePdf
                        if endNumber > totalNumberOfPictures:
                            endNumber = totalNumberOfPictures
                        filename = []
                        if numberOfEntitiesInOnePdf > 1:
                            filename = os.path.join(save_pdf_path, outputPdfName + "_" + nameOfPart + "_" + str(
                                numberOfPicturesAdded + 1) + '-' + str(endNumber) + "_of_" + str(
                                totalNumberOfPictures) + ".pdf")
                        elif numberOfEntitiesInOnePdf == 1:
                            filename = os.path.join(save_pdf_path, outputPdfName + "_" + nameOfPart + "_" + str(
                                numberOfPicturesAdded + 1) + "_of_" + str(totalNumberOfPictures) + ".pdf")
                        c = canvas.Canvas(filename)
                    img = utils.ImageReader(iPicture)
                    imagesize = img.getSize()
                    # noinspection PyUnboundLocalVariable
                    c.setPageSize(imagesize)
                    c.drawImage(iPicture, 0, 0)
                    c.showPage()
                    numberOfPicturesAdded += 1
                    if (numberOfPicturesAdded % numberOfEntitiesInOnePdf) == 0:
                        c.save()
                        # noinspection PyUnboundLocalVariable
                        print("created", filename)
                if (numberOfPicturesAdded % numberOfEntitiesInOnePdf) != 0:
                    c.save()
                    print("created", filename)
            elif splitType == "none":
                filename = os.path.join(save_pdf_path, outputPdfName + ".pdf")
                c = canvas.Canvas(filename)
                for iPicture in picturesInsideFolderWithPictures:
                    img = utils.ImageReader(iPicture)
                    imagesize = img.getSize()
                    c.setPageSize(imagesize)
                    c.drawImage(iPicture, 0, 0)
                    c.showPage()
                c.save()
                print("created", filename)
            else:
                print("Wrong splitType value")
        else:
            print("No pictures found.")
        return


def main():
    outputPdfName = 'rapidTweakRegressionH0_soc3p2_chg_mon__rapidTweakRegressionH0_soc3p2_chg_mon-2024-04-29T05-08-45.pdf'
    save_pdf_path = '/home/daveg/.local/SOC_Particle/g20240331/figures'
    pathToPictures = '.'
    splitType = 'picture'
    numberOfEntitiesInOnePdf = 9
    listWithImagesExtensions = ['png']
    picturesAreInRootFolder = True
    nameOfPart = 'picture'

    unite_pictures_into_pdf(outputPdfName, save_pdf_path, pathToPictures, splitType, numberOfEntitiesInOnePdf,
                            listWithImagesExtensions, picturesAreInRootFolder, nameOfPart)


# ----------------------------------------------------------------------
if __name__ == "__main__":
    main()
