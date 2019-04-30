import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

#Importing the dataset
dataset=pd.read_csv("dataset1.csv")
X=dataset.iloc[:,:-1].values
Y=dataset.iloc[:,6].values


from sklearn.tree import DecisionTreeClassifier
classifier = DecisionTreeClassifier(random_state = 0)
classifier.fit(X, Y)


from sklearn import tree
import graphviz
dot_data = tree.export_graphviz(classifier, feature_names=["GyX","GyY","GyZ","AcX","AcY","AcZ"],out_file=None,    
    filled=True, class_names=["0","1"],rounded=True, special_characters=True) 
graph = graphviz.Source(dot_data)  
graph.render('dtree_render',view=True)

























